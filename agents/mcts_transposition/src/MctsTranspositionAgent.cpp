#include "tetris_env/mcts/TranspositionRolloutAgent.hpp"

#include "tetris_env/MctsRolloutAgent.hpp"

namespace {

MctsParams makeTranspositionParams(MctsParams params) {
    params.rolloutPolicy = MctsRolloutPolicy::Greedy;
    params.valueFunction = MctsValueFunction::ScoreDelta;
    params.useTranspositionTable = true;
    return params;
}

} // namespace

MctsTranspositionAgent::MctsTranspositionAgent(const MctsParams& params)
    : impl_(makeTranspositionParams(params)) {}

Action MctsTranspositionAgent::chooseAction(const TetrisEnv& env) {
    return impl_.chooseAction(env);
}
