#pragma once

#include "tetris_env/MctsRolloutAgent.hpp"

class MctsGreedyRolloutAgent : public Agent {
public:
    explicit MctsGreedyRolloutAgent(const MctsParams& params = MctsParams());

    Action chooseAction(const TetrisEnv& env) override;
    void onEpisodeStart() override { impl_.onEpisodeStart(); }
    void onEpisodeEnd() override { impl_.onEpisodeEnd(); }

private:
    MctsRolloutAgent impl_;
};
