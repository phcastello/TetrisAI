#pragma once

#include <random>

#include "Agent.hpp"

class RandomAgent : public Agent {
public:
    RandomAgent();

    Action chooseAction(const TetrisEnv& env) override;

private:
    std::mt19937 rng_;
};
