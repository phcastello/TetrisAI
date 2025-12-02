#pragma once

#include <cstdint>
#include <optional>
#include <random>

#include "tetris_env/Agent.hpp"
#include "tetris_env/TetrisEnv.hpp"

struct MctsParams {
    int iterations = 0;
    int maxDepth = 0;
    double exploration = 0.0;
    std::optional<std::uint32_t> seed{};
};

class MctsRolloutAgent : public Agent {
public:
    explicit MctsRolloutAgent(const MctsParams& params = MctsParams());

    Action chooseAction(const TetrisEnv& env) override;

private:
    MctsParams params_;
    std::mt19937 rng_;
};
