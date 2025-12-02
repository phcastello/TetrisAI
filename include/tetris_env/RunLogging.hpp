#pragma once

#include <string>

#include "tetris_env/EpisodeReport.hpp"

namespace tetris {

// Builds a run identifier timestamp in the format YYYYMMDD_HHMMSSmmm.
std::string makeRunIdTimestamp();

// Appends the episode report to agents/<agentDir>/run_<runId><suffix>.csv, creating the file with header if needed.
void appendEpisodeReportToRunFile(const EpisodeReport& rep, const std::string& agentDir, const std::string& filenameSuffix = "");

} // namespace tetris
