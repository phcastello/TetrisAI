#include "tetris_env/MctsRolloutAgent.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

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

} // namespace

MctsRolloutAgent::MctsRolloutAgent(const MctsParams& params) : params_(params) {
    if (params_.seed.has_value()) {
        rng_.seed(*params_.seed);
    } else {
        rng_.seed(std::random_device{}());
    }
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

    std::vector<Node> nodes;
    nodes.emplace_back();

    const int rootIndex = 0;
    Node& root = nodes[rootIndex];
    root.parent = -1;
    root.actionFromParent = Action{};
    root.visits = 0;
    root.totalValue = 0.0;
    root.terminal = false;
    root.untriedActions = rootActions;

    for (int i = 0; i < params_.iterations; ++i) {
        TetrisEnv sim = env.clone();
        int nodeIndex = rootIndex;
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
                const Node& child = nodes[childIndex];
                const double q = child.visits > 0 ? (child.totalValue / child.visits) : 0.0;
                const double u = params_.exploration *
                                 std::sqrt(parentVisitsLog / (1.0 + static_cast<double>(child.visits)));
                const double score = q + u;

                if (score > bestScore) {
                    bestScore = score;
                    bestChild = childIndex;
                }
            }

            const Action& a = nodes[bestChild].actionFromParent;
            const StepResult r = sim.step(a);
            accumulatedReward += static_cast<double>(r.reward);
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
        if (!selectedNode.terminal && depth < params_.maxDepth && !selectedNode.untriedActions.empty()) {
            std::uniform_int_distribution<std::size_t> dist(0, selectedNode.untriedActions.size() - 1);
            const std::size_t actionIdx = dist(rng_);
            Action a = selectedNode.untriedActions[actionIdx];
            selectedNode.untriedActions[actionIdx] = selectedNode.untriedActions.back();
            selectedNode.untriedActions.pop_back();

            const StepResult r = sim.step(a);
            accumulatedReward += static_cast<double>(r.reward);
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
            nodes[nodeIndex].children.push_back(childIndex);
            nodeIndex = childIndex;
        }

        // Rollout
        const Node& rolloutNode = nodes[nodeIndex];
        if (!rolloutNode.terminal && depth < params_.maxDepth) {
            while (!sim.isGameOver() && depth < params_.maxDepth) {
                const auto actions = sim.getValidActions();
                if (actions.empty()) {
                    break;
                }

                std::uniform_int_distribution<std::size_t> dist(0, actions.size() - 1);
                const Action& a = actions[dist(rng_)];
                const StepResult r = sim.step(a);
                accumulatedReward += static_cast<double>(r.reward);
                ++depth;

                if (r.done) {
                    break;
                }
            }
        }

        // Backpropagation
        int current = nodeIndex;
        while (current != -1) {
            Node& n = nodes[current];
            n.visits += 1;
            n.totalValue += accumulatedReward;
            current = n.parent;
        }
    }

    Action bestAction = rootActions.front();
    double bestValue = -std::numeric_limits<double>::infinity();
    bool foundVisitedChild = false;

    const Node& finalRoot = nodes[rootIndex];
    for (int childIndex : finalRoot.children) {
        const Node& child = nodes[childIndex];
        if (child.visits == 0) {
            continue;
        }

        const double meanValue = child.totalValue / child.visits;
        if (meanValue > bestValue) {
            bestValue = meanValue;
            bestAction = child.actionFromParent;
            foundVisitedChild = true;
        }
    }

    if (!foundVisitedChild) {
        if (!finalRoot.untriedActions.empty()) {
            std::uniform_int_distribution<std::size_t> dist(0, finalRoot.untriedActions.size() - 1);
            bestAction = finalRoot.untriedActions[dist(rng_)];
        } else if (!finalRoot.children.empty()) {
            bestAction = nodes[finalRoot.children.front()].actionFromParent;
        }
    }

    return bestAction;
}
