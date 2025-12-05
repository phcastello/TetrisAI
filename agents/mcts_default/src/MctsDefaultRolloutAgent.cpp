#include "tetris_env/mcts/DefaultRolloutAgent.hpp"

#include "tetris_env/MctsRolloutAgent.hpp"

namespace {

MctsParams makeDefaultParams(MctsParams params) {
    params.rolloutPolicy = MctsRolloutPolicy::Random;
    params.valueFunction = MctsValueFunction::ScoreDelta;
    params.useTranspositionTable = false;
    return params;
}

} // namespace

MctsDefaultRolloutAgent::MctsDefaultRolloutAgent(const MctsParams& params)
    : impl_(makeDefaultParams(params)) {}

Action MctsDefaultRolloutAgent::chooseAction(const TetrisEnv& env) {
    return impl_.chooseAction(env);
}
