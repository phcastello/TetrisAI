#include "tetris_env/mcts/GreedyRolloutAgent.hpp"

#include "tetris_env/MctsRolloutAgent.hpp"

namespace {

MctsParams makeGreedyParams(MctsParams params) {
    params.rolloutPolicy = MctsRolloutPolicy::Greedy;
    params.valueFunction = MctsValueFunction::ScoreDelta;
    params.useTranspositionTable = false;
    return params;
}

} // namespace

MctsGreedyRolloutAgent::MctsGreedyRolloutAgent(const MctsParams& params)
    : impl_(makeGreedyParams(params)) {}

Action MctsGreedyRolloutAgent::chooseAction(const TetrisEnv& env) {
    return impl_.chooseAction(env);
}
