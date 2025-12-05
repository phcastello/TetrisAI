#include "tetris_env/MctsConfig.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

namespace tetris {
namespace {

std::string trim(const std::string& text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }
    if (start == text.size()) {
        return "";
    }
    std::size_t end = text.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(text[end])) != 0) {
        --end;
    }
    return text.substr(start, end - start + 1);
}

bool tryParseInt(const std::string& value, int& out) {
    try {
        out = std::stoi(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool tryParseDouble(const std::string& value, double& out) {
    try {
        out = std::stod(value);
        return true;
    } catch (...) {
        return false;
    }
}

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

bool tryParseUint32(const std::string& value, std::uint32_t& out) {
    try {
        const unsigned long parsed = std::stoul(value);
        if (parsed > std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }
        out = static_cast<std::uint32_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool tryParseBool(const std::string& value, bool& out) {
    const std::string lower = toLower(value);
    if (lower == "true" || lower == "yes" || lower == "1" || lower == "on") {
        out = true;
        return true;
    }
    if (lower == "false" || lower == "no" || lower == "0" || lower == "off") {
        out = false;
        return true;
    }
    return false;
}

} // namespace

std::optional<std::filesystem::path> findMctsConfigPath(const std::string& agentDir) {
    namespace fs = std::filesystem;
    std::vector<std::string> dirNames;
    dirNames.push_back(agentDir);
    if (agentDir == "mcts_rollout") {
        dirNames.push_back("mcts_greedy");
        dirNames.push_back("mcts_default");
        dirNames.push_back("mcts_transposition");
    } else if (agentDir == "mcts_greedy" || agentDir == "mcts_default" || agentDir == "mcts_transposition") {
        dirNames.push_back("mcts_rollout");
    }

    std::vector<fs::path> candidates;
    auto pushUnique = [&](const fs::path& candidate) {
        if (std::find(candidates.begin(), candidates.end(), candidate) == candidates.end()) {
            candidates.push_back(candidate);
        }
    };

    for (const auto& dir : dirNames) {
        pushUnique(fs::path("agents") / dir / "config.yaml");
        pushUnique(fs::path("config") / (dir + ".yaml"));
        pushUnique(fs::path("..") / "agents" / dir / "config.yaml");
        pushUnique(fs::path("..") / "config" / (dir + ".yaml"));
        pushUnique(fs::path("agents") / (dir + ".yaml"));
    }

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            return candidate;
        }
    }
    return std::nullopt;
}

bool loadMctsParamsFromYaml(const std::filesystem::path& filepath, ::MctsParams& params) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Erro: nao foi possivel abrir config MCTS em " << filepath << '\n';
        return false;
    }

    bool hasIterations = false;
    bool hasMaxDepth = false;
    bool hasExploration = false;

    std::string line;
    while (std::getline(file, line)) {
        const auto commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line = trim(line);
        if (line.empty()) {
            continue;
        }

        const auto colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, colonPos));
        const std::string value = trim(line.substr(colonPos + 1));
        if (key.empty() || value.empty()) {
            continue;
        }

        if (key == "iterations") {
            int parsed = params.iterations;
            if (tryParseInt(value, parsed) && parsed > 0) {
                params.iterations = parsed;
                hasIterations = true;
            }
        } else if (key == "maxDepth" || key == "rollout_depth") {
            int parsed = params.maxDepth;
            if (tryParseInt(value, parsed) && parsed > 0) {
                params.maxDepth = parsed;
                hasMaxDepth = true;
            }
        } else if (key == "exploration" || key == "uct_c") {
            double parsed = params.exploration;
            if (tryParseDouble(value, parsed) && parsed > 0.0) {
                params.exploration = parsed;
                hasExploration = true;
            }
        } else if (key == "threads" || key == "num_threads") {
            int parsed = params.threads;
            if (tryParseInt(value, parsed) && parsed > 0) {
                params.threads = parsed;
            }
        } else if (key == "seed") {
            std::uint32_t parsedSeed{};
            if (tryParseUint32(value, parsedSeed)) {
                params.seed = parsedSeed;
            }
        } else if (key == "score_limit" || key == "maxScore" || key == "max_score") {
            int parsed = 0;
            if (tryParseInt(value, parsed) && parsed > 0) {
                params.scoreLimit = parsed;
            }
        } else if (key == "time_limit_seconds" || key == "time_limit" || key == "max_time_seconds") {
            double parsed = 0.0;
            if (tryParseDouble(value, parsed) && parsed > 0.0) {
                params.timeLimitSeconds = parsed;
            }
        } else if (key == "rollout_policy") {
            const std::string lower = toLower(value);
            if (lower == "greedy") {
                params.rolloutPolicy = MctsRolloutPolicy::Greedy;
            } else if (lower == "random") {
                params.rolloutPolicy = MctsRolloutPolicy::Random;
            }
        } else if (key == "reward_mode") {
            const std::string lower = toLower(value);
            if (lower == "score") {
                params.valueFunction = MctsValueFunction::ScoreDelta;
            } else if (lower == "greedy") {
                params.valueFunction = MctsValueFunction::GreedyHeuristic;
            }
        } else if (key == "use_transposition_table" || key == "use_tt") {
            bool parsed = false;
            if (tryParseBool(value, parsed)) {
                params.useTranspositionTable = parsed;
            }
        } else if (key == "tt_max_entries") {
            int parsed = 0;
            if (tryParseInt(value, parsed) && parsed >= 0) {
                params.ttMaxEntries = static_cast<std::size_t>(parsed);
            }
        }
    }

    if (!hasIterations || !hasMaxDepth || !hasExploration) {
        std::cerr << "Erro: config MCTS incompleta em " << filepath
                  << " (iterations, rollout_depth/maxDepth e exploration/uct_c sao obrigatorios)\n";
        return false;
    }

    return true;
}

std::string buildMctsConfigString(const ::MctsParams& params) {
    std::ostringstream oss;
    oss << "iterations=" << params.iterations
        << " maxDepth=" << params.maxDepth
        << " exploration=" << params.exploration
        << " threads=" << params.threads
        << " seed=";
    if (params.seed.has_value()) {
        oss << *params.seed;
    } else {
        oss << "random";
    }
    oss << " scoreLimit=";
    if (params.scoreLimit.has_value()) {
        oss << *params.scoreLimit;
    } else {
        oss << "none";
    }
    oss << " timeLimitSeconds=";
    if (params.timeLimitSeconds.has_value()) {
        oss << *params.timeLimitSeconds;
    } else {
        oss << "none";
    }
    const char* rolloutStr = params.rolloutPolicy == MctsRolloutPolicy::Greedy ? "greedy" : "random";
    const char* rewardStr =
        params.valueFunction == MctsValueFunction::ScoreDelta ? "score" : "greedy";
    oss << " rollout=" << rolloutStr
        << " reward=" << rewardStr
        << " tt=" << (params.useTranspositionTable ? "on" : "off")
        << " tt_max_entries=" << params.ttMaxEntries;
    return oss.str();
}

} // namespace tetris
