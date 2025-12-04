#pragma once

#include <random>
#include <unordered_map>

#include "tetris_env/Agent.hpp"
#include "tetris_env/TetrisEnv.hpp"
#include "tetris_env/mcts/MctsParams.hpp"

class MctsTranspositionAgent : public Agent {
public:
    explicit MctsTranspositionAgent(const MctsParams& params = MctsParams());

    Action chooseAction(const TetrisEnv& env) override;

private:
    MctsParams params_;
    std::mt19937 rng_;
};
