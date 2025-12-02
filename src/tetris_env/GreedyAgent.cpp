#include "tetris_env/GreedyAgent.hpp"

#include <cmath>
#include <limits>
#include <vector>

Action GreedyAgent::chooseAction(const TetrisEnv& env) {
    if (env.isGameOver()) {
        return Action{};
    }

    const auto actions = env.getValidActions();
    if (actions.empty()) {
        return Action{};
    }

    double bestValue = -std::numeric_limits<double>::infinity();
    Action bestAction = actions.front();

    for (const auto& action : actions) {
        TetrisEnv sim = env.clone();
        const StepResult stepResult = sim.step(action);
        const double value = evaluateAfterAction(env, sim, stepResult);

        if (value > bestValue) {
            bestValue = value;
            bestAction = action;
        }
    }

    return bestAction;
}

double GreedyAgent::evaluateAfterAction(const TetrisEnv& before,
                                        const TetrisEnv& after,
                                        const StepResult& stepResult) const {
    const BoardFeatures beforeFeatures = computeBoardFeatures(before);
    const BoardFeatures afterFeatures = computeBoardFeatures(after);

    const int linesCleared = stepResult.linesCleared;
    const int holesDelta = afterFeatures.holes - beforeFeatures.holes;

    const double wLines = 1.0;
    const double wHoles = 4.0;
    const double wTotalHeight = 0.5;
    const double wBumpiness = 0.3;
    const double wNewHoles = 2.0;
    const double wScore = 0.01;

    double value = 0.0;
    value += wLines * static_cast<double>(linesCleared);
    value += wScore * static_cast<double>(stepResult.scoreDelta);
    value -= wHoles * static_cast<double>(afterFeatures.holes);
    value -= wTotalHeight * static_cast<double>(afterFeatures.totalHeight);
    value -= wBumpiness * static_cast<double>(afterFeatures.bumpiness);
    if (holesDelta > 0) {
        value -= wNewHoles * static_cast<double>(holesDelta);
    }

    return value;
}

GreedyAgent::BoardFeatures GreedyAgent::computeBoardFeatures(const TetrisEnv& env) const {
    const auto& board = env.getBoard();
    const int width = env.getBoardWidth();
    const int height = env.getBoardHeight();

    std::vector<int> heights(static_cast<std::size_t>(width), 0);
    int holes = 0;

    const auto& grid = board.data();

    for (int x = 0; x < width; ++x) {
        bool seenBlock = false;
        int columnHeight = 0;

        for (int y = 0; y < height; ++y) {
            const bool occupied = grid[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] != 0;
            if (occupied) {
                if (!seenBlock) {
                    seenBlock = true;
                    columnHeight = height - y;
                }
            } else if (seenBlock) {
                ++holes;
            }
        }

        heights[static_cast<std::size_t>(x)] = columnHeight;
    }

    int totalHeight = 0;
    int maxHeight = 0;
    for (int h : heights) {
        totalHeight += h;
        if (h > maxHeight) {
            maxHeight = h;
        }
    }

    int bumpiness = 0;
    for (int x = 0; x + 1 < width; ++x) {
        bumpiness += std::abs(heights[static_cast<std::size_t>(x)] - heights[static_cast<std::size_t>(x + 1)]);
    }

    BoardFeatures f{};
    f.totalHeight = totalHeight;
    f.maxHeight = maxHeight;
    f.holes = holes;
    f.bumpiness = bumpiness;
    return f;
}
