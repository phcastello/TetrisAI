#include "tetris_env/RandomAgent.hpp"

#include <random>

RandomAgent::RandomAgent() : rng_(std::random_device{}()) {}

Action RandomAgent::chooseAction(const TetrisEnv& env) {
    const auto actions = env.getValidActions();
    if (actions.empty()) {
        return Action{};
    }

    std::uniform_int_distribution<std::size_t> dist(0, actions.size() - 1);
    return actions[dist(rng_)];
}
