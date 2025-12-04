#pragma once

namespace tetris {

enum class PlayMode {
    Human = 0,
    RandomAI = 1,
    GreedyAI = 2,
    MctsGreedyAI = 3,
    MctsDefaultAI = 4,
    MctsTranspositionAI = 5,
};

} // namespace tetris
