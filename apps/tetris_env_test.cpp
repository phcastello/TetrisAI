#include <iostream>

#include "tetris_env/RandomAgent.hpp"
#include "tetris_env/TetrisEnv.hpp"

int main() {
    TetrisEnv env;
    RandomAgent agent;

    const int numEpisodes = 5;

    for (int ep = 0; ep < numEpisodes; ++ep) {
        env.reset();
        agent.onEpisodeStart();

        while (!env.isGameOver()) {
            Action action = agent.chooseAction(env);
            StepResult result = env.step(action);
            if (result.done) {
                break;
            }
        }

        agent.onEpisodeEnd();

        std::cout << "Episode " << ep
                  << " finished. Score = " << env.getScore()
                  << ", lines = " << env.getTotalLinesCleared()
                  << std::endl;
    }

    return 0;
}
