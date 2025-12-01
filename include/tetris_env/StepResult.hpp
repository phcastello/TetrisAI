#pragma once

struct StepResult {
    int reward = 0;        // recompensa genérica; igual a linesCleared por enquanto
    int linesCleared = 0;  // número de linhas limpas nesta jogada
    int scoreDelta = 0;    // incremento de score nesta jogada
    bool done = false;     // true se o jogo terminou após a ação
};
