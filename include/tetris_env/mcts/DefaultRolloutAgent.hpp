#pragma once

#include <random>

#include "tetris_env/Agent.hpp"
#include "tetris_env/TetrisEnv.hpp"
#include "tetris_env/mcts/MctsParams.hpp"

class MctsDefaultRolloutAgent : public Agent {
public:
    explicit MctsDefaultRolloutAgent(const MctsParams& params = MctsParams());

    Action chooseAction(const TetrisEnv& env) override;

private:
    MctsParams params_;
    std::mt19937 rng_;
};
