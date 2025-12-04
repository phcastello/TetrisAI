#include "tetris_env/MctsConfig.hpp"

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

} // namespace

std::optional<std::filesystem::path> findMctsConfigPath(const std::string& agentDir) {
    namespace fs = std::filesystem;
    std::vector<fs::path> candidates{
        fs::path("agents") / agentDir / "config.yaml",
        fs::path("config") / (agentDir + ".yaml"),
        fs::path("..") / "agents" / agentDir / "config.yaml",
        fs::path("..") / "config" / (agentDir + ".yaml"),
        fs::path("agents") / (agentDir + ".yaml"),
    };

    if (agentDir == "mcts_greedy") {
        // Suporte retroativo ao nome antigo.
        candidates.push_back("agents/mcts_rollout/config.yaml");
        candidates.push_back("config/mcts_rollout.yaml");
        candidates.push_back("../agents/mcts_rollout/config.yaml");
        candidates.push_back("../config/mcts_rollout.yaml");
        candidates.push_back("agents/mcts_rollout.yaml");
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
    return oss.str();
}

} // namespace tetris
