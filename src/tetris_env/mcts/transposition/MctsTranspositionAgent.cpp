#include "tetris_env/mcts/TranspositionRolloutAgent.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tetris/EngineConfig.hpp"
#include "tetris_env/GreedyAgent.hpp"
#include "tetris_env/StepResult.hpp"

namespace {

struct Node {
    int parent = -1;
    Action actionFromParent{};

    int visits = 0;
    double totalValue = 0.0;

    bool terminal = false;
    std::vector<Action> untriedActions;
    std::vector<int> children;
};

struct SearchResult {
    std::vector<int> visits;
    std::vector<double> totalValue;
};

struct StateKey {
    std::array<std::array<int, tetris::engine_cfg::fieldWidth>, tetris::engine_cfg::fieldHeight> board{};
    bool hasActive = false;
    int activeId = -1;
    int rotation = 0;
    int originX = 0;
    int originY = 0;
    bool canHold = false;
    bool hasHold = false;
    int holdPiece = -1;
    std::array<int, tetris::engine_cfg::queuePreviewCount> nextQueue{};
    int nextQueueCount = 0;

    bool operator==(const StateKey& other) const {
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
};

struct StateKeyHash {
    std::size_t operator()(const StateKey& key) const {
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
};

using TranspositionTable = std::unordered_map<StateKey, std::pair<int, double>, StateKeyHash>;

bool actionsEqual(const Action& a, const Action& b) {
    return a.rotation == b.rotation && a.targetX == b.targetX && a.useHold == b.useHold;
}

int findRootActionIndex(const std::vector<Action>& actions, const Action& target) {
    for (std::size_t i = 0; i < actions.size(); ++i) {
        if (actionsEqual(actions[i], target)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

StateKey makeKey(const TetrisEnv& env) {
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

SearchResult runSearch(const TetrisEnv& env,
                       const std::vector<Action>& rootActions,
                       const MctsParams& params,
                       int iterations,
                       std::mt19937& rng,
                       TranspositionTable& table) {
    SearchResult result{};
    result.visits.assign(rootActions.size(), 0);
    result.totalValue.assign(rootActions.size(), 0.0);

    if (iterations <= 0 || params.maxDepth <= 0) {
        return result;
    }

    auto stepValue = [](const StepResult& r) { return static_cast<double>(r.scoreDelta); };

    std::vector<Node> nodes;
    std::vector<StateKey> nodeKeys;
    nodes.reserve(static_cast<std::size_t>(iterations) + 1);
    nodeKeys.reserve(static_cast<std::size_t>(iterations) + 1);

    nodes.emplace_back();
    nodeKeys.push_back(makeKey(env));

    Node& root = nodes.front();
    root.parent = -1;
    root.actionFromParent = Action{};
    root.visits = 0;
    root.totalValue = 0.0;
    root.terminal = false;
    root.untriedActions = rootActions;

    GreedyAgent rolloutPolicy;

    for (int i = 0; i < iterations; ++i) {
        TetrisEnv sim = env.clone();
        int nodeIndex = 0;
        double accumulatedReward = 0.0;
        int depth = 0;

        // Selection
        while (true) {
            Node& node = nodes[nodeIndex];

            if (node.terminal || depth >= params.maxDepth) {
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
                const Node& child = nodes[childIndex];
                const double q = child.visits > 0 ? (child.totalValue / child.visits) : 0.0;
                const double u = params.exploration *
                                 std::sqrt(parentVisitsLog / (1.0 + static_cast<double>(child.visits)));
                const double score = q + u;

                if (score > bestScore) {
                    bestScore = score;
                    bestChild = childIndex;
                }
            }

            const Action& a = nodes[bestChild].actionFromParent;
            const StepResult r = sim.step(a);
            accumulatedReward += stepValue(r);
            ++depth;

            if (r.done || sim.isGameOver()) {
                nodes[bestChild].terminal = true;
                nodeIndex = bestChild;
                break;
            }

            nodeIndex = bestChild;
        }

        // Expansion
        Node& selectedNode = nodes[nodeIndex];
        if (!selectedNode.terminal && depth < params.maxDepth && !selectedNode.untriedActions.empty()) {
            std::uniform_int_distribution<std::size_t> dist(0, selectedNode.untriedActions.size() - 1);
            const std::size_t actionIdx = dist(rng);
            Action a = selectedNode.untriedActions[actionIdx];
            selectedNode.untriedActions[actionIdx] = selectedNode.untriedActions.back();
            selectedNode.untriedActions.pop_back();

            const StepResult r = sim.step(a);
            accumulatedReward += stepValue(r);
            ++depth;

            Node child{};
            child.parent = nodeIndex;
            child.actionFromParent = a;
            child.visits = 0;
            child.totalValue = 0.0;
            child.terminal = r.done || sim.isGameOver();

            if (!child.terminal) {
                child.untriedActions = sim.getValidActions();
                if (child.untriedActions.empty()) {
                    child.terminal = true;
                }
            }

            const int childIndex = static_cast<int>(nodes.size());
            nodes.push_back(std::move(child));
            nodeKeys.push_back(makeKey(sim));

            // Puxa estatisticas ja vistas para inicializar o filho.
            const auto it = table.find(nodeKeys.back());
            if (it != table.end()) {
                nodes.back().visits = it->second.first;
                nodes.back().totalValue = it->second.second;
            }

            nodes[nodeIndex].children.push_back(childIndex);
            nodeIndex = childIndex;
        }

        // Rollout (greedy, com fallback aleatorio se necessario).
        const Node& rolloutNode = nodes[nodeIndex];
        if (!rolloutNode.terminal && depth < params.maxDepth) {
            while (!sim.isGameOver() && depth < params.maxDepth) {
                const auto actions = sim.getValidActions();
                if (actions.empty()) {
                    break;
                }

                Action a = rolloutPolicy.chooseAction(sim);

                bool valid = false;
                for (const auto& candidate : actions) {
                    if (actionsEqual(candidate, a)) {
                        valid = true;
                        break;
                    }
                }
                if (!valid) {
                    std::uniform_int_distribution<std::size_t> dist(0, actions.size() - 1);
                    a = actions[dist(rng)];
                }

                const StepResult r = sim.step(a);
                accumulatedReward += stepValue(r);
                ++depth;

                if (r.done) {
                    break;
                }
            }
        }

        // Backpropagation (tambem atualiza tabela de transposicao).
        int current = nodeIndex;
        while (current != -1) {
            Node& n = nodes[current];
            n.visits += 1;
            n.totalValue += accumulatedReward;

            auto& entry = table[nodeKeys[static_cast<std::size_t>(current)]];
            entry.first += 1;
            entry.second += accumulatedReward;

            current = n.parent;
        }
    }

    for (int childIndex : nodes.front().children) {
        const Node& child = nodes[childIndex];
        const int idx = findRootActionIndex(rootActions, child.actionFromParent);
        if (idx >= 0) {
            result.visits[static_cast<std::size_t>(idx)] += child.visits;
            result.totalValue[static_cast<std::size_t>(idx)] += child.totalValue;
        }
    }

    return result;
}

} // namespace

MctsTranspositionAgent::MctsTranspositionAgent(const MctsParams& params) : params_(params) {
    if (params_.seed.has_value()) {
        rng_.seed(*params_.seed);
    } else {
        rng_.seed(std::random_device{}());
    }
}

Action MctsTranspositionAgent::chooseAction(const TetrisEnv& env) {
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
        TranspositionTable table;
        partial[0] = runSearch(env, rootActions, params_, totalIterations, rng_, table);
    } else {
        std::vector<std::thread> workers;
        workers.reserve(static_cast<std::size_t>(workerCount));

        std::vector<std::uint32_t> seeds(static_cast<std::size_t>(workerCount));
        for (int i = 0; i < workerCount; ++i) {
            seeds[static_cast<std::size_t>(i)] = rng_();
        }

        for (int i = 0; i < workerCount; ++i) {
            const int iterationsForThread = baseIterations + (i < remainder ? 1 : 0);
            workers.emplace_back([&, i, iterationsForThread]() {
                std::mt19937 localRng(seeds[static_cast<std::size_t>(i)]);
                TranspositionTable localTable;
                partial[static_cast<std::size_t>(i)] =
                    runSearch(env, rootActions, params_, iterationsForThread, localRng, localTable);
            });
        }

        for (auto& w : workers) {
            if (w.joinable()) {
                w.join();
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
        std::uniform_int_distribution<std::size_t> dist(0, rootActions.size() - 1);
        bestAction = rootActions[dist(rng_)];
    }

    return bestAction;
}
