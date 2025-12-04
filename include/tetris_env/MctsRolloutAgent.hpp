#pragma once

#include <cstdint>
#include <optional>
#include <random>

#include "tetris_env/Agent.hpp"
#include "tetris_env/StepResult.hpp"
#include "tetris_env/TetrisEnv.hpp"

struct MctsParams {
    int iterations = 0;
    int maxDepth = 0;
    double exploration = 0.0;
    int threads = 1;
    std::optional<std::uint32_t> seed{};
    std::optional<int> scoreLimit{};
    std::optional<double> timeLimitSeconds{};
};

class MctsRolloutAgent : public Agent {
public:
    explicit MctsRolloutAgent(const MctsParams& params = MctsParams());

    Action chooseAction(const TetrisEnv& env) override;

private:
    MctsParams params_;
    std::mt19937 rng_;
};
