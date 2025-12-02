#pragma once

#include "tetris_env/Agent.hpp"
#include "tetris_env/TetrisEnv.hpp"
#include "tetris_env/StepResult.hpp"

class GreedyAgent : public Agent {
public:
    GreedyAgent() = default;
    Action chooseAction(const TetrisEnv& env) override;

private:
    // Avalia o estado depois de aplicar uma ação
    double evaluateAfterAction(const TetrisEnv& before,
                               const TetrisEnv& after,
                               const StepResult& stepResult) const;

    // Função auxiliar para extrair features do tabuleiro
    struct BoardFeatures {
        int totalHeight = 0;
        int maxHeight = 0;
        int holes = 0;
        int bumpiness = 0;
    };

    BoardFeatures computeBoardFeatures(const TetrisEnv& env) const;
};
