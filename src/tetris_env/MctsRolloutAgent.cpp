#include "tetris_env/MctsRolloutAgent.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <thread>
#include <utility>

#include "tetris/EngineConfig.hpp"
#include "tetris_env/BoardHeuristic.hpp"
#include "tetris_env/GreedyAgent.hpp"

namespace {

constexpr std::size_t kDefaultTtEntries = 200000;

bool actionsEqual(const Action& a, const Action& b) {
    return a.rotation == b.rotation && a.targetX == b.targetX && a.useHold == b.useHold;
}

Action randomAction(const std::vector<Action>& actions, std::mt19937& rng) {
    if (actions.empty()) {
        return Action{};
    }
    std::uniform_int_distribution<std::size_t> dist(0, actions.size() - 1);
    return actions[dist(rng)];
}

} // namespace

bool MctsRolloutAgent::StateKey::operator==(const StateKey& other) const {
    return board == other.board &&
           hasActive == other.hasActive &&
           activeId == other.activeId &&
           rotation == other.rotation &&
           originX == other.originX &&
           originY == other.originY &&
           canHold == other.canHold &&
           hasHold == other.hasHold &&
           holdPiece == other.holdPiece &&
           nextQueue == other.nextQueue &&
           nextQueueCount == other.nextQueueCount;
}

std::size_t MctsRolloutAgent::StateKeyHash::operator()(const StateKey& key) const {
    std::size_t h = 0;
    auto combine = [&](std::size_t value) {
        h ^= value + 0x9e3779b9 + (h << 6) + (h >> 2);
    };

    for (const auto& row : key.board) {
        for (int cell : row) {
            combine(std::hash<int>{}(cell));
        }
    }

    combine(std::hash<int>{}(static_cast<int>(key.hasActive)));
    combine(std::hash<int>{}(key.activeId));
    combine(std::hash<int>{}(key.rotation));
    combine(std::hash<int>{}(key.originX));
    combine(std::hash<int>{}(key.originY));
    combine(std::hash<int>{}(static_cast<int>(key.canHold)));
    combine(std::hash<int>{}(static_cast<int>(key.hasHold)));
    combine(std::hash<int>{}(key.holdPiece));
    combine(std::hash<int>{}(key.nextQueueCount));
    for (int val : key.nextQueue) {
        combine(std::hash<int>{}(val));
    }

    return h;
}

MctsRolloutAgent::MctsRolloutAgent(MctsParams params) : params_(std::move(params)) {
    if (params_.seed.has_value()) {
        rng_.seed(*params_.seed);
    } else {
        rng_.seed(std::random_device{}());
    }

    ttMaxEntries_ = params_.ttMaxEntries == 0 ? kDefaultTtEntries : params_.ttMaxEntries;
}

void MctsRolloutAgent::onEpisodeStart() {
    if (params_.useTranspositionTable) {
        transpositionTable_.clear();
    }
}

double MctsRolloutAgent::evalScoreDelta(const StepResult& r) const {
    return static_cast<double>(r.scoreDelta);
}

double MctsRolloutAgent::evalGreedyHeuristic(const tetris_env::BoardFeatures& before,
                                             const tetris_env::BoardFeatures& after,
                                             const StepResult& r) const {
    return tetris_env::evaluateGreedyStep(before, after, r);
}

double MctsRolloutAgent::stepValue(const StepResult& r,
                                   const tetris_env::BoardFeatures* beforeFeatures,
                                   const tetris_env::BoardFeatures* afterFeatures) const {
    switch (params_.valueFunction) {
        case MctsValueFunction::ScoreDelta:
            return evalScoreDelta(r);
        case MctsValueFunction::GreedyHeuristic:
            if (beforeFeatures != nullptr && afterFeatures != nullptr) {
                return evalGreedyHeuristic(*beforeFeatures, *afterFeatures, r);
            }
            return 0.0;
        default:
            return 0.0;
    }
}

Action MctsRolloutAgent::rolloutAction(const TetrisEnv& sim,
                                       const std::vector<Action>& validActions,
                                       std::mt19937& rng,
                                       GreedyAgent& greedyPolicy) const {
    if (validActions.empty()) {
        return Action{};
    }

    if (params_.rolloutPolicy == MctsRolloutPolicy::Random) {
        return randomAction(validActions, rng);
    }

    Action chosen = greedyPolicy.chooseAction(sim);
    for (const auto& candidate : validActions) {
        if (actionsEqual(candidate, chosen)) {
            return chosen;
        }
    }

    return randomAction(validActions, rng);
}

MctsRolloutAgent::StateKey MctsRolloutAgent::makeKey(const TetrisEnv& env) const {
    StateKey key{};

    const auto& grid = env.getBoard().data();
    for (int y = 0; y < tetris::engine_cfg::fieldHeight; ++y) {
        for (int x = 0; x < tetris::engine_cfg::fieldWidth; ++x) {
            key.board[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] =
                grid[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
        }
    }

    const auto& game = env.game();
    key.canHold = game.canHold();
    key.hasHold = game.hasHoldPiece();
    key.holdPiece = key.hasHold ? game.holdPiece() : -1;

    key.hasActive = game.hasActivePiece();
    if (key.hasActive) {
        const auto& active = game.activePiece();
        key.activeId = active.id;
        key.rotation = active.rotation;
        key.originX = active.origin.x;
        key.originY = active.origin.y;
    }

    key.nextQueue.fill(-1);
    const auto preview = env.getNextQueue();
    key.nextQueueCount = static_cast<int>(std::min<std::size_t>(preview.size(), key.nextQueue.size()));
    for (int i = 0; i < key.nextQueueCount; ++i) {
        key.nextQueue[static_cast<std::size_t>(i)] = preview[static_cast<std::size_t>(i)];
    }

    return key;
}

MctsRolloutAgent::SearchResult MctsRolloutAgent::runSearch(const TetrisEnv& env,
                                                           const std::vector<Action>& rootActions,
                                                           int iterations,
                                                           std::mt19937& rng,
                                                           TranspositionTable* table) {
    SearchResult result{};
    result.visits.assign(rootActions.size(), 0);
    result.totalValue.assign(rootActions.size(), 0.0);

    if (iterations <= 0 || params_.maxDepth <= 0) {
        return result;
    }

    const bool useTranspositions = params_.useTranspositionTable && table != nullptr && ttMaxEntries_ > 0;
    const std::size_t tableLimit = useTranspositions ? ttMaxEntries_ : 0;

    std::vector<Node> nodes;
    nodes.reserve(static_cast<std::size_t>(iterations) + 1);
    nodes.emplace_back();

    std::vector<StateKey> nodeKeys;
    if (useTranspositions) {
        nodeKeys.reserve(static_cast<std::size_t>(iterations) + 1);
        nodeKeys.push_back(makeKey(env));
        const auto it = table->find(nodeKeys.back());
        if (it != table->end()) {
            nodes.front().visits = it->second.visits;
            nodes.front().totalValue = it->second.totalValue;
        }
    }

    Node& root = nodes.front();
    root.parent = -1;
    root.actionFromParent = Action{};
    root.terminal = false;
    root.untriedActions = rootActions;

    GreedyAgent rolloutGreedyPolicy;

    for (int i = 0; i < iterations; ++i) {
        TetrisEnv sim = env.clone();
        int nodeIndex = 0;
        double accumulatedReward = 0.0;
        int depth = 0;

        // Selection
        while (true) {
            Node& node = nodes[nodeIndex];

            if (node.terminal || depth >= params_.maxDepth) {
                break;
            }

            if (!node.untriedActions.empty()) {
                break;
            }

            if (node.children.empty()) {
                node.terminal = true;
                break;
            }

            int bestChild = node.children.front();
            double bestScore = -std::numeric_limits<double>::infinity();
            const double parentVisitsLog = std::log(std::max(1, node.visits));

            for (int childIndex : node.children) {
                const Node& child = nodes[static_cast<std::size_t>(childIndex)];
                const double q = child.visits > 0 ? (child.totalValue / child.visits) : 0.0;
                const double u = params_.exploration *
                                 std::sqrt(parentVisitsLog / (1.0 + static_cast<double>(child.visits)));
                const double score = q + u;

                if (score > bestScore) {
                    bestScore = score;
                    bestChild = childIndex;
                }
            }

            const Action& a = nodes[static_cast<std::size_t>(bestChild)].actionFromParent;
            tetris_env::BoardFeatures beforeFeatures{};
            tetris_env::BoardFeatures afterFeatures{};
            const tetris_env::BoardFeatures* beforePtr = nullptr;
            const tetris_env::BoardFeatures* afterPtr = nullptr;
            if (params_.valueFunction == MctsValueFunction::GreedyHeuristic) {
                beforeFeatures = tetris_env::computeBoardFeatures(sim);
                beforePtr = &beforeFeatures;
            }
            const StepResult r = sim.step(a);
            if (params_.valueFunction == MctsValueFunction::GreedyHeuristic) {
                afterFeatures = tetris_env::computeBoardFeatures(sim);
                afterPtr = &afterFeatures;
            }
            accumulatedReward += stepValue(r, beforePtr, afterPtr);
            ++depth;

            if (r.done || sim.isGameOver()) {
                nodes[static_cast<std::size_t>(bestChild)].terminal = true;
                nodeIndex = bestChild;
                break;
            }

            nodeIndex = bestChild;
        }

        // Expansion
        Node& selectedNode = nodes[static_cast<std::size_t>(nodeIndex)];
        if (!selectedNode.terminal && depth < params_.maxDepth && !selectedNode.untriedActions.empty()) {
            std::uniform_int_distribution<std::size_t> dist(0, selectedNode.untriedActions.size() - 1);
            const std::size_t actionIdx = dist(rng);
            Action a = selectedNode.untriedActions[actionIdx];
            selectedNode.untriedActions[actionIdx] = selectedNode.untriedActions.back();
            selectedNode.untriedActions.pop_back();

            tetris_env::BoardFeatures beforeFeatures{};
            tetris_env::BoardFeatures afterFeatures{};
            const tetris_env::BoardFeatures* beforePtr = nullptr;
            const tetris_env::BoardFeatures* afterPtr = nullptr;
            if (params_.valueFunction == MctsValueFunction::GreedyHeuristic) {
                beforeFeatures = tetris_env::computeBoardFeatures(sim);
                beforePtr = &beforeFeatures;
            }
            const StepResult r = sim.step(a);
            if (params_.valueFunction == MctsValueFunction::GreedyHeuristic) {
                afterFeatures = tetris_env::computeBoardFeatures(sim);
                afterPtr = &afterFeatures;
            }

            accumulatedReward += stepValue(r, beforePtr, afterPtr);
            ++depth;

            Node child{};
            child.parent = nodeIndex;
            child.actionFromParent = a;
            child.terminal = r.done || sim.isGameOver();

            if (!child.terminal) {
                child.untriedActions = sim.getValidActions();
                if (child.untriedActions.empty()) {
                    child.terminal = true;
                }
            }

            const int childIndex = static_cast<int>(nodes.size());
            nodes.push_back(std::move(child));
            if (useTranspositions) {
                nodeKeys.push_back(makeKey(sim));
                const auto it = table->find(nodeKeys.back());
                if (it != table->end()) {
                    nodes.back().visits = it->second.visits;
                    nodes.back().totalValue = it->second.totalValue;
                }
            }

            nodes[static_cast<std::size_t>(nodeIndex)].children.push_back(childIndex);
            nodeIndex = childIndex;
        }

        // Rollout
        const Node& rolloutNode = nodes[static_cast<std::size_t>(nodeIndex)];
        if (!rolloutNode.terminal && depth < params_.maxDepth) {
            while (!sim.isGameOver() && depth < params_.maxDepth) {
                const auto actions = sim.getValidActions();
                if (actions.empty()) {
                    break;
                }

                Action a = rolloutAction(sim, actions, rng, rolloutGreedyPolicy);

                tetris_env::BoardFeatures beforeFeatures{};
                tetris_env::BoardFeatures afterFeatures{};
                const tetris_env::BoardFeatures* beforePtr = nullptr;
                const tetris_env::BoardFeatures* afterPtr = nullptr;
                if (params_.valueFunction == MctsValueFunction::GreedyHeuristic) {
                    beforeFeatures = tetris_env::computeBoardFeatures(sim);
                    beforePtr = &beforeFeatures;
                }
                const StepResult r = sim.step(a);
                if (params_.valueFunction == MctsValueFunction::GreedyHeuristic) {
                    afterFeatures = tetris_env::computeBoardFeatures(sim);
                    afterPtr = &afterFeatures;
                }

                accumulatedReward += stepValue(r, beforePtr, afterPtr);
                ++depth;

                if (r.done) {
                    break;
                }
            }
        }

        // Backpropagation
        int current = nodeIndex;
        while (current != -1) {
            Node& n = nodes[static_cast<std::size_t>(current)];
            n.visits += 1;
            n.totalValue += accumulatedReward;

            if (useTranspositions) {
                const StateKey& key = nodeKeys[static_cast<std::size_t>(current)];
                auto it = table->find(key);
                if (it != table->end()) {
                    it->second.visits += 1;
                    it->second.totalValue += accumulatedReward;
                } else if (table->size() < tableLimit) {
                    table->emplace(key, TranspositionEntry{1, accumulatedReward});
                }
            }

            current = n.parent;
        }
    }

    for (int childIndex : nodes.front().children) {
        const Node& child = nodes[static_cast<std::size_t>(childIndex)];
        for (std::size_t i = 0; i < rootActions.size(); ++i) {
            if (actionsEqual(rootActions[i], child.actionFromParent)) {
                result.visits[i] += child.visits;
                result.totalValue[i] += child.totalValue;
                break;
            }
        }
    }

    return result;
}

Action MctsRolloutAgent::chooseAction(const TetrisEnv& env) {
    if (params_.iterations <= 0 || params_.maxDepth <= 0 || params_.exploration <= 0.0) {
        return Action{};
    }

    if (env.isGameOver()) {
        return Action{};
    }

    const auto rootActions = env.getValidActions();
    if (rootActions.empty()) {
        return Action{};
    }

    const int totalIterations = params_.iterations;
    const int maxThreads = std::max(1, params_.threads);
    const int workerCount = std::max(1, std::min(totalIterations, maxThreads));
    const int baseIterations = totalIterations / workerCount;
    const int remainder = totalIterations % workerCount;

    std::vector<SearchResult> partial(static_cast<std::size_t>(workerCount));

    if (workerCount == 1) {
        TranspositionTable* tablePtr = params_.useTranspositionTable ? &transpositionTable_ : nullptr;
        partial.front() = runSearch(env, rootActions, totalIterations, rng_, tablePtr);
    } else {
        std::vector<std::thread> workers;
        workers.reserve(static_cast<std::size_t>(workerCount));

        std::vector<std::uint32_t> seeds(static_cast<std::size_t>(workerCount));
        for (int i = 0; i < workerCount; ++i) {
            seeds[static_cast<std::size_t>(i)] = rng_();
        }

        std::vector<TranspositionTable> localTables;
        if (params_.useTranspositionTable) {
            localTables.resize(static_cast<std::size_t>(workerCount));
        }

        for (int i = 0; i < workerCount; ++i) {
            const int iterationsForThread = baseIterations + (i < remainder ? 1 : 0);
            workers.emplace_back([&, i, iterationsForThread]() {
                std::mt19937 localRng(seeds[static_cast<std::size_t>(i)]);
                TranspositionTable* tablePtr = params_.useTranspositionTable
                    ? &localTables[static_cast<std::size_t>(i)]
                    : nullptr;
                partial[static_cast<std::size_t>(i)] =
                    runSearch(env, rootActions, iterationsForThread, localRng, tablePtr);
            });
        }

        for (auto& w : workers) {
            if (w.joinable()) {
                w.join();
            }
        }

        if (params_.useTranspositionTable && ttMaxEntries_ > 0) {
            for (const auto& table : localTables) {
                for (const auto& kv : table) {
                    auto it = transpositionTable_.find(kv.first);
                    if (it != transpositionTable_.end()) {
                        it->second.visits += kv.second.visits;
                        it->second.totalValue += kv.second.totalValue;
                    } else if (transpositionTable_.size() < ttMaxEntries_) {
                        transpositionTable_.emplace(kv.first, kv.second);
                    }
                }
            }
        }
    }

    std::vector<int> totalVisits(rootActions.size(), 0);
    std::vector<double> totalValues(rootActions.size(), 0.0);
    for (const auto& res : partial) {
        for (std::size_t i = 0; i < rootActions.size(); ++i) {
            totalVisits[i] += res.visits[i];
            totalValues[i] += res.totalValue[i];
        }
    }

    Action bestAction = rootActions.front();
    double bestValue = -std::numeric_limits<double>::infinity();
    bool foundVisitedChild = false;

    for (std::size_t i = 0; i < rootActions.size(); ++i) {
        if (totalVisits[i] == 0) {
            continue;
        }

        const double meanValue = totalValues[i] / static_cast<double>(totalVisits[i]);
        if (meanValue > bestValue) {
            bestValue = meanValue;
            bestAction = rootActions[i];
            foundVisitedChild = true;
        }
    }

    if (!foundVisitedChild) {
        bestAction = randomAction(rootActions, rng_);
    }

    return bestAction;
}
