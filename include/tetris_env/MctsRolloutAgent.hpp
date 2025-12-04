#pragma once

#include "tetris_env/mcts/GreedyRolloutAgent.hpp"
#include "tetris_env/mcts/MctsParams.hpp"

// Alias mantido para compatibilidade: o antigo "MctsRolloutAgent"
// agora aponta para a versão com rollout guiado pela heuristica Greedy.
using MctsRolloutAgent = MctsGreedyRolloutAgent;
