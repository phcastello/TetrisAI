#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "tetris_env/MctsRolloutAgent.hpp"

namespace tetris {

// Attempts to locate a MCTS configuration file in common locations relative to the current working directory.
std::optional<std::filesystem::path> findMctsConfigPath();

// Loads MctsParams from the given YAML file (simple key: value format).
bool loadMctsParamsFromYaml(const std::filesystem::path& filepath, ::MctsParams& params);

// Returns a concise string representation of the MCTS configuration for logging.
std::string buildMctsConfigString(const ::MctsParams& params);

} // namespace tetris
