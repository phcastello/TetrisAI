#include "tetris_env/RunLogging.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace tetris {

std::string makeRunIdTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S") << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

void appendEpisodeReportToRunFile(const EpisodeReport& rep, const std::string& agentDir) {
    namespace fs = std::filesystem;

    fs::path dir = fs::path("agents") / agentDir;
    fs::create_directories(dir);

    const std::string filename = "run_" + rep.runId + ".csv";
    fs::path filepath = dir / filename;

    const bool isNewFile = !fs::exists(filepath);

    std::ofstream file(filepath, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo de log: " << filepath << '\n';
        return;
    }

    if (isNewFile) {
        file << "run_id,episode_index,agent_name,mode_name,score,total_lines,total_turns,holds_used,elapsed_seconds,agent_config\n";
    }

    file << rep.runId << ','
         << rep.episodeIndex << ','
         << rep.agentName << ','
         << rep.modeName << ','
         << rep.score << ','
         << rep.totalLines << ','
         << rep.totalTurns << ','
         << rep.holdsUsed << ','
         << std::fixed << std::setprecision(2) << rep.elapsedSeconds << ','
         << rep.agentConfig
         << '\n';
}

} // namespace tetris
