#include "tetris_env/GreedyAgent.hpp"

#include <cmath>
#include <limits>
#include <vector>

#include "tetris_env/BoardHeuristic.hpp"

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
    const tetris_env::BoardFeatures beforeFeatures = tetris_env::computeBoardFeatures(before);
    const tetris_env::BoardFeatures afterFeatures = tetris_env::computeBoardFeatures(after);
    return tetris_env::evaluateGreedyStep(beforeFeatures, afterFeatures, stepResult);
}
