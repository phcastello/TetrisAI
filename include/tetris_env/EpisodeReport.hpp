#pragma once

#include <string>

namespace tetris {

struct EpisodeReport {
    std::string agentName;
    std::string modeName;
    std::string agentConfig;

    std::string runId;
    int episodeIndex = 1;

    int score = 0;
    int totalLines = 0;
    int totalTurns = 0;
    int holdsUsed = 0;

    float elapsedSeconds = 0.0f;
};

} // namespace tetris
