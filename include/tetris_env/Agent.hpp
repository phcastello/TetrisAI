#pragma once

#include "Action.hpp"
#include "TetrisEnv.hpp"

class Agent {
public:
    virtual ~Agent() = default;

    virtual Action chooseAction(const TetrisEnv& env) = 0;

    virtual void onEpisodeStart() {}
    virtual void onEpisodeEnd() {}
};
