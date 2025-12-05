#pragma once

#include "tetris_env/TetrisEnv.hpp"
#include "tetris_env/StepResult.hpp"

namespace tetris_env {

struct BoardFeatures {
    int totalHeight = 0;
    int maxHeight = 0;
    int holes = 0;
    int bumpiness = 0;
};

BoardFeatures computeBoardFeatures(const TetrisEnv& env);

double evaluateGreedyStep(const BoardFeatures& before,
                          const BoardFeatures& after,
                          const StepResult& stepResult);

} // namespace tetris_env
