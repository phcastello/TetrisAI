#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "tetris_env/Agent.hpp"
#include "tetris_env/GreedyAgent.hpp"
#include "tetris_env/MctsConfig.hpp"
#include "tetris_env/RandomAgent.hpp"
#include "tetris_env/RunLogging.hpp"
#include "tetris_env/TetrisEnv.hpp"
#include "tetris_env/MctsRolloutAgent.hpp"
#include "tetris_env/mcts/DefaultRolloutAgent.hpp"
#include "tetris_env/mcts/GreedyRolloutAgent.hpp"
#include "tetris_env/mcts/TranspositionRolloutAgent.hpp"

struct AgentBatchConfig {
    std::string name;
    std::string type;
    int episodes = 0;
    std::optional<std::string> mctsConfigPath;
};

struct BatchConfig {
    std::filesystem::path baseDir{};
    int threads = 0;
    std::vector<AgentBatchConfig> agents;
};

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

void printUsage() {
    std::cout << "Uso: tetris_batch_runner [caminho_config_yaml]\n\n"
              << "- Se nenhum caminho for informado, usa \"config/batch_runs.yaml\".\n"
              << "- O arquivo YAML deve ter o formato:\n\n"
              << "    threads: 4\n"
              << "    agents:\n"
              << "      - name: random_baseline\n"
              << "        type: random\n"
              << "        episodes: 100\n"
              << "      - name: greedy_baseline\n"
              << "        type: greedy\n"
              << "        episodes: 100\n"
              << "      - name: mcts_greedy\n"
              << "        type: mcts_greedy   # ou mcts_rollout (alias)\n"
              << "        episodes: 100\n"
              << "        mcts_config: agents/mcts_greedy/config.yaml\n"
              << "      - name: mcts_default_rollout\n"
              << "        type: mcts_default  # rollouts aleatorias\n"
              << "        episodes: 100\n"
              << "      - name: mcts_tt\n"
              << "        type: mcts_transposition  # usa tabela de transposicao\n"
              << "        episodes: 100\n"
              << "        mcts_config: agents/mcts_transposition/config.yaml\n\n"
              << "- Os resultados de cada agente sao gravados em:\n"
              << "    agents/<agent_dir>/run_<runId>.csv\n"
              << "- Agentes MCTS rodam episodios de forma sequencial e usam o numero de\n"
              << "  threads configurado para paralelizar o proprio jogo.\n";
}

std::optional<BatchConfig> loadBatchConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Erro: nao foi possivel abrir o arquivo de configuracao em " << path << '\n';
        return std::nullopt;
    }

    BatchConfig config{};
    const std::filesystem::path absoluteConfig = std::filesystem::absolute(path);
    config.baseDir = absoluteConfig.parent_path();

    bool inAgentsSection = false;
    bool agentStarted = false;
    AgentBatchConfig currentAgent{};

    std::string line;
    int lineNumber = 0;

    auto flushCurrentAgent = [&](int currentLine) -> bool {
        if (!agentStarted) {
            return true;
        }
        if (currentAgent.name.empty()) {
            std::cerr << "Erro: agente sem 'name' proximo da linha " << currentLine << '\n';
            return false;
        }
        if (currentAgent.type != "random" &&
            currentAgent.type != "mcts_rollout" &&
            currentAgent.type != "mcts_greedy" &&
            currentAgent.type != "mcts_default" &&
            currentAgent.type != "mcts_transposition" &&
            currentAgent.type != "greedy") {
            std::cerr << "Erro: tipo invalido para o agente '" << currentAgent.name
                      << "' (linha " << currentLine
                      << "). Use 'random', 'greedy', 'mcts_greedy', 'mcts_default', 'mcts_transposition' "
                         "ou 'mcts_rollout'.\n";
            return false;
        }
        if (currentAgent.episodes <= 0) {
            std::cerr << "Erro: 'episodes' deve ser > 0 para o agente '" << currentAgent.name
                      << "' (linha " << currentLine << ").\n";
            return false;
        }
        config.agents.push_back(currentAgent);
        agentStarted = false;
        currentAgent = AgentBatchConfig{};
        return true;
    };

    while (std::getline(file, line)) {
        ++lineNumber;
        const auto commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line = trim(line);
        if (line.empty()) {
            continue;
        }

        if (!inAgentsSection) {
            if (line.rfind("threads", 0) == 0) {
                const auto colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    const std::string value = trim(line.substr(colonPos + 1));
                    int parsed = config.threads;
                    if (tryParseInt(value, parsed)) {
                        config.threads = parsed;
                    } else {
                        std::cerr << "Aviso: nao foi possivel interpretar 'threads' na linha " << lineNumber << '\n';
                    }
                }
                continue;
            }

            if (line == "agents:") {
                inAgentsSection = true;
                continue;
            }

            // Linha desconhecida fora da secao de agentes: ignorar silenciosamente.
            continue;
        }

        if (line[0] == '-') {
            if (!flushCurrentAgent(lineNumber)) {
                return std::nullopt;
            }

            currentAgent = AgentBatchConfig{};
            agentStarted = true;

            std::string afterDash = trim(line.substr(1));
            if (!afterDash.empty()) {
                const auto colonPos = afterDash.find(':');
                if (colonPos != std::string::npos) {
                    const std::string key = trim(afterDash.substr(0, colonPos));
                    const std::string value = trim(afterDash.substr(colonPos + 1));
                    if (key == "name") {
                        currentAgent.name = value;
                    } else if (key == "type") {
                        currentAgent.type = value;
                    } else if (key == "episodes") {
                        int parsed = 0;
                        if (tryParseInt(value, parsed)) {
                            currentAgent.episodes = parsed;
                        }
                    } else if (key == "mcts_config") {
                        currentAgent.mctsConfigPath = value;
                    }
                }
            }
            continue;
        }

        if (!agentStarted) {
            std::cerr << "Erro de formato: esperava '- ' para iniciar um agente na linha " << lineNumber << '\n';
            return std::nullopt;
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

        if (key == "name") {
            currentAgent.name = value;
        } else if (key == "type") {
            currentAgent.type = value;
        } else if (key == "episodes") {
            int parsed = 0;
            if (tryParseInt(value, parsed)) {
                currentAgent.episodes = parsed;
            }
        } else if (key == "mcts_config") {
            currentAgent.mctsConfigPath = value;
        }
    }

    if (!flushCurrentAgent(lineNumber)) {
        return std::nullopt;
    }

    if (!inAgentsSection) {
        std::cerr << "Erro: campo 'agents:' nao encontrado no arquivo de configuracao.\n";
        return std::nullopt;
    }

    if (config.agents.empty()) {
        std::cerr << "Erro: nenhum agente configurado em " << path << '\n';
        return std::nullopt;
    }

    return config;
}

std::string agentDirForType(const std::string& type) {
    if (type == "mcts_greedy" || type == "mcts_rollout") {
        return "mcts_greedy";
    }
    if (type == "mcts_default") {
        return "mcts_default";
    }
    if (type == "mcts_transposition") {
        return "mcts_transposition";
    }
    if (type == "greedy") {
        return "heuristic_greedy";
    }
    return "random_agent";
}

std::string agentFilenameSuffixForType(const std::string& type) {
    if (type == "mcts_greedy" || type == "mcts_rollout") {
        return "_MCTS_greedy";
    }
    if (type == "mcts_default") {
        return "_MCTS_default";
    }
    if (type == "mcts_transposition") {
        return "_MCTS_tt";
    }
    if (type == "greedy") {
        return "_greedy";
    }
    if (type == "random") {
        return "_random";
    }
    return "";
}

bool isMctsType(const std::string& type) {
    return type.find("mcts") != std::string::npos;
}

std::optional<std::filesystem::path> resolveMctsConfigForAgent(const AgentBatchConfig& agentCfg,
                                                               const std::filesystem::path& configBaseDir,
                                                               const std::string& agentDir) {
    namespace fs = std::filesystem;

    if (agentCfg.mctsConfigPath.has_value()) {
        fs::path provided = *agentCfg.mctsConfigPath;
        std::vector<fs::path> candidates;
        if (provided.is_absolute()) {
            candidates.push_back(provided);
        } else {
            candidates.push_back(provided);
            if (!configBaseDir.empty()) {
                candidates.push_back(configBaseDir / provided);
                const fs::path baseParent = configBaseDir.parent_path();
                if (!baseParent.empty()) {
                    candidates.push_back(baseParent / provided);
                }
            }
        }

        for (const auto& candidate : candidates) {
            if (fs::exists(candidate)) {
                return candidate;
            }
        }

        return provided;
    }

    return tetris::findMctsConfigPath(agentDir);
}

bool runEpisodesForAgent(const AgentBatchConfig& agentCfg,
                         unsigned int maxConcurrentThreads,
                         const std::filesystem::path& configBaseDir) {
    const std::string runId = tetris::makeRunIdTimestamp();
    const std::string normalizedType = agentCfg.type == "mcts_rollout" ? "mcts_greedy" : agentCfg.type;
    const std::string agentDir = agentDirForType(agentCfg.type);
    const std::string agentFilenameSuffix = agentFilenameSuffixForType(agentCfg.type);
    const bool isMctsAgent = isMctsType(normalizedType);
    const unsigned int threadsForEpisodes = isMctsAgent
        ? 1u
        : std::max(1u, std::min(maxConcurrentThreads, static_cast<unsigned int>(agentCfg.episodes)));
    const unsigned int mctsThreadBudget = isMctsAgent ? std::max(1u, maxConcurrentThreads) : 1u;

    std::optional<MctsParams> mctsParams{};
    std::string agentConfigString;

    if (isMctsAgent) {
        const auto configPathOpt = resolveMctsConfigForAgent(agentCfg, configBaseDir, agentDir);
        if (!configPathOpt.has_value()) {
            std::cerr << "Erro: config do MCTS nao encontrada. Defina mcts_config ou crie agents/" << agentDir
                      << "/config.yaml.\n";
            return false;
        }

        std::filesystem::path configPath = *configPathOpt;
        if (!std::filesystem::exists(configPath)) {
            std::cerr << "Erro: caminho de config do MCTS nao existe: " << configPath << '\n';
            return false;
        }

        MctsParams params{};
        if (!tetris::loadMctsParamsFromYaml(configPath, params)) {
            return false;
        }

        params.threads = static_cast<int>(mctsThreadBudget);
        mctsParams = params;
        agentConfigString = tetris::buildMctsConfigString(params);

        std::cout << "Config MCTS carregada de " << configPath << " para tipo " << normalizedType << '\n';
    }

    std::vector<tetris::EpisodeReport> reports(static_cast<std::size_t>(agentCfg.episodes));
    std::vector<std::thread> workers;
    workers.reserve(std::min<unsigned int>(threadsForEpisodes, static_cast<unsigned int>(agentCfg.episodes)));
    std::mutex logMutex;
    std::atomic_bool success{true};
    std::atomic<int> completedEpisodes{0};

    const std::optional<MctsParams> paramsOpt = mctsParams;
    const std::string agentNameCopy = agentCfg.name;
    const std::string modeName = "HeadlessBatch";
    const std::string runIdCopy = runId;
    const std::string agentConfigCopy = agentConfigString;
    const std::string agentTypeCopy = normalizedType;
    const bool isMctsAgentCopy = isMctsAgent;
    const int totalEpisodesCopy = agentCfg.episodes;
    const unsigned int mctsThreadsCopy = mctsThreadBudget;
    const std::optional<int> scoreLimitCopy = paramsOpt.has_value() ? paramsOpt->scoreLimit : std::nullopt;
    const std::optional<double> timeLimitCopy = paramsOpt.has_value() ? paramsOpt->timeLimitSeconds : std::nullopt;

    auto launchEpisode = [&](int episodeIndex) {
        workers.emplace_back([&, episodeIndex]() {
            TetrisEnv env{};
            env.reset();

            std::unique_ptr<Agent> agent;
            if (agentTypeCopy == "random") {
                agent = std::make_unique<RandomAgent>();
            } else if (agentTypeCopy == "mcts_greedy") {
                if (!paramsOpt.has_value()) {
                    std::lock_guard<std::mutex> lock(logMutex);
                    std::cerr << "Erro interno: MCTS params nao carregados.\n";
                    success.store(false, std::memory_order_relaxed);
                    return;
                }
                MctsParams paramsCopy = *paramsOpt;
                paramsCopy.threads = static_cast<int>(mctsThreadsCopy);
                agent = std::make_unique<MctsGreedyRolloutAgent>(paramsCopy);
            } else if (agentTypeCopy == "mcts_default") {
                if (!paramsOpt.has_value()) {
                    std::lock_guard<std::mutex> lock(logMutex);
                    std::cerr << "Erro interno: MCTS params nao carregados.\n";
                    success.store(false, std::memory_order_relaxed);
                    return;
                }
                MctsParams paramsCopy = *paramsOpt;
                paramsCopy.threads = static_cast<int>(mctsThreadsCopy);
                agent = std::make_unique<MctsDefaultRolloutAgent>(paramsCopy);
            } else if (agentTypeCopy == "mcts_transposition") {
                if (!paramsOpt.has_value()) {
                    std::lock_guard<std::mutex> lock(logMutex);
                    std::cerr << "Erro interno: MCTS params nao carregados.\n";
                    success.store(false, std::memory_order_relaxed);
                    return;
                }
                MctsParams paramsCopy = *paramsOpt;
                paramsCopy.threads = static_cast<int>(mctsThreadsCopy);
                agent = std::make_unique<MctsTranspositionAgent>(paramsCopy);
            } else if (agentTypeCopy == "greedy") {
                agent = std::make_unique<GreedyAgent>();
            } else {
                std::lock_guard<std::mutex> lock(logMutex);
                std::cerr << "Tipo de agente desconhecido: " << agentTypeCopy << '\n';
                success.store(false, std::memory_order_relaxed);
                return;
            }

            agent->onEpisodeStart();
            const auto start = std::chrono::steady_clock::now();
            std::string endReason = "game_over";
            while (!env.isGameOver()) {
                if (scoreLimitCopy.has_value() && env.getScore() >= *scoreLimitCopy) {
                    endReason = "score_limit";
                    break;
                }

                if (timeLimitCopy.has_value()) {
                    const double elapsedSeconds =
                        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                    if (elapsedSeconds >= *timeLimitCopy) {
                        endReason = "time_limit";
                        break;
                    }
                }

                const Action action = agent->chooseAction(env);
                const StepResult result = env.step(action);
                if (result.done) {
                    break;
                }
            }
            const auto end = std::chrono::steady_clock::now();
            agent->onEpisodeEnd();

            tetris::EpisodeReport report{};
            report.agentName = agentNameCopy;
            report.modeName = modeName;
            report.agentConfig = agentConfigCopy;
            report.runId = runIdCopy;
            report.episodeIndex = episodeIndex;
            report.score = env.getScore();
            report.totalLines = env.getTotalLinesCleared();
            report.totalTurns = env.getTurnNumber();
            report.holdsUsed = env.getHoldsUsed();
            report.elapsedSeconds = std::chrono::duration<float>(end - start).count();
            report.endReason = endReason;

            reports[static_cast<std::size_t>(episodeIndex - 1)] = report;

            {
                std::lock_guard<std::mutex> lock(logMutex);
                const int finished = completedEpisodes.fetch_add(1, std::memory_order_relaxed) + 1;
                std::cout << "[Agente " << agentNameCopy << "] episodio " << episodeIndex
                          << " concluido: score=" << report.score
                          << " linhas=" << report.totalLines
                          << " turns=" << report.totalTurns
                          << " tempo=" << report.elapsedSeconds << "s";
                if (endReason == "score_limit") {
                    std::cout << " (encerrado por limite de score)";
                } else if (endReason == "time_limit") {
                    std::cout << " (encerrado por limite de tempo)";
                }
                if (isMctsAgentCopy) {
                    const double progress = static_cast<double>(finished) * 100.0 /
                                             static_cast<double>(std::max(1, totalEpisodesCopy));
                    std::cout << " progresso=" << finished << '/' << totalEpisodesCopy
                              << " (" << progress << "%)";
                }
                std::cout << '\n';
            }
        });
    };

    int launched = 0;
    while (launched < agentCfg.episodes) {
        while (launched < agentCfg.episodes &&
               workers.size() < threadsForEpisodes) {
            launchEpisode(launched + 1);
            ++launched;
        }

        for (auto& w : workers) {
            if (w.joinable()) {
                w.join();
            }
        }
        workers.clear();

        if (!success.load(std::memory_order_relaxed)) {
            break;
        }
    }

    if (!success.load(std::memory_order_relaxed)) {
        return false;
    }

    std::sort(reports.begin(), reports.end(),
              [](const tetris::EpisodeReport& a, const tetris::EpisodeReport& b) {
                  return a.episodeIndex < b.episodeIndex;
              });

    for (const auto& rep : reports) {
        tetris::appendEpisodeReportToRunFile(rep, agentDir, agentFilenameSuffix);
    }

    double sumScore = 0.0;
    double sumLines = 0.0;
    for (const auto& rep : reports) {
        sumScore += static_cast<double>(rep.score);
        sumLines += static_cast<double>(rep.totalLines);
    }

    const double avgScore = reports.empty() ? 0.0 : sumScore / static_cast<double>(reports.size());
    const double avgLines = reports.empty() ? 0.0 : sumLines / static_cast<double>(reports.size());

    std::cout << "[Agente " << agentCfg.name << "] run_id=" << runId
              << " episodios=" << reports.size()
              << " threads=" << threadsForEpisodes;
    if (isMctsAgent) {
        std::cout << " mcts_threads=" << mctsThreadBudget;
    }
    std::cout << " avg_score=" << avgScore
              << " avg_lines=" << avgLines
              << " -> agents/" << agentDir << "/run_" << runId << agentFilenameSuffix << ".csv\n";

    return true;
}

} // namespace

int main(int argc, char** argv) {
    std::string configPath = "config/batch_runs.yaml";
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }
        configPath = arg;
    }

    const auto configOpt = loadBatchConfig(configPath);
    if (!configOpt.has_value()) {
        printUsage();
        return 1;
    }

    const BatchConfig config = *configOpt;

    unsigned int maxThreads = 0;
    if (config.threads > 0) {
        maxThreads = static_cast<unsigned int>(config.threads);
    } else {
        maxThreads = std::thread::hardware_concurrency();
    }
    if (maxThreads == 0) {
        maxThreads = 1;
    }

    for (const auto& agentCfg : config.agents) {
        const bool isMctsAgent = isMctsType(agentCfg.type);
        unsigned int threadsForThisAgent = maxThreads;
        if (!isMctsAgent && agentCfg.episodes < static_cast<int>(threadsForThisAgent)) {
            threadsForThisAgent = static_cast<unsigned int>(std::max(1, agentCfg.episodes));
        }

        if (isMctsAgent) {
            std::cout << "Executando agente '" << agentCfg.name << "' (" << agentCfg.type
                      << ") com episodios sequenciais; ate " << threadsForThisAgent
                      << " thread(s) por jogo MCTS...\n";
        } else {
            std::cout << "Executando agente '" << agentCfg.name << "' (" << agentCfg.type
                      << ") com ate " << threadsForThisAgent
                      << " jogo(s) em paralelo...\n";
        }

        const bool ok = runEpisodesForAgent(agentCfg, threadsForThisAgent, config.baseDir);
        if (!ok) {
            std::cerr << "Execucao interrompida para o agente '" << agentCfg.name << "'.\n";
            return 1;
        }
    }

    return 0;
}
