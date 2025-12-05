#include "tetris_env/BoardHeuristic.hpp"

#include <cmath>
#include <vector>

namespace tetris_env {

BoardFeatures computeBoardFeatures(const TetrisEnv& env) {
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

double evaluateGreedyStep(const BoardFeatures& before,
                          const BoardFeatures& after,
                          const StepResult& stepResult) {
    const int linesCleared = stepResult.linesCleared;
    const int holesDelta = after.holes - before.holes;

    const double wLines = 1.0;
    const double wHoles = 4.0;
    const double wTotalHeight = 0.5;
    const double wBumpiness = 0.3;
    const double wNewHoles = 2.0;
    const double wScore = 0.01;

    double value = 0.0;
    value += wLines * static_cast<double>(linesCleared);
    value += wScore * static_cast<double>(stepResult.scoreDelta);
    value -= wHoles * static_cast<double>(after.holes);
    value -= wTotalHeight * static_cast<double>(after.totalHeight);
    value -= wBumpiness * static_cast<double>(after.bumpiness);
    if (holesDelta > 0) {
        value -= wNewHoles * static_cast<double>(holesDelta);
    }

    return value;
}

} // namespace tetris_env
