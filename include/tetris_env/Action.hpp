#pragma once

struct Action {
    int rotation = 0;  // rotação final da peça (0..3)
    int targetX = 0;   // coordenada origin.x da peça (canto sup. esquerdo do bounding box) ao travar
    bool useHold = false;  // true se o hold deve ser aplicado antes de posicionar
};
