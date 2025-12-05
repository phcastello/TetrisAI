#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <unordered_map>
#include <vector>

#include "tetris_env/Agent.hpp"
#include "tetris/EngineConfig.hpp"
#include "tetris_env/TetrisEnv.hpp"
#include "tetris_env/StepResult.hpp"

class GreedyAgent;
namespace tetris_env {
struct BoardFeatures;
}

enum class MctsRolloutPolicy {
    Random,
    Greedy
};

enum class MctsValueFunction {
    ScoreDelta,
    GreedyHeuristic
};

struct MctsParams {
    int iterations = 0;
    int maxDepth = 0;
    double exploration = 0.0;
    int threads = 1;
    std::optional<std::uint32_t> seed{};
    std::optional<int> scoreLimit{};
    std::optional<double> timeLimitSeconds{};

    MctsRolloutPolicy rolloutPolicy = MctsRolloutPolicy::Greedy;
    MctsValueFunction valueFunction = MctsValueFunction::ScoreDelta;
    bool useTranspositionTable = false;
    std::size_t ttMaxEntries = 0; // 0 = usar um default interno razoavel
};

class MctsRolloutAgent : public Agent {
public:
    explicit MctsRolloutAgent(MctsParams params = MctsParams());

    Action chooseAction(const TetrisEnv& env) override;
    void onEpisodeStart() override;
    void onEpisodeEnd() override {}

private:
    struct Node {
        int parent = -1;
        Action actionFromParent{};

        int visits = 0;
        double totalValue = 0.0;

        bool terminal = false;
        std::vector<Action> untriedActions;
        std::vector<int> children;
    };

    struct SearchResult {
        std::vector<int> visits;
        std::vector<double> totalValue;
    };

    struct StateKey {
        std::array<std::array<int, tetris::engine_cfg::fieldWidth>, tetris::engine_cfg::fieldHeight> board{};
        bool hasActive = false;
        int activeId = -1;
        int rotation = 0;
        int originX = 0;
        int originY = 0;
        bool canHold = false;
        bool hasHold = false;
        int holdPiece = -1;
        std::array<int, tetris::engine_cfg::queuePreviewCount> nextQueue{};
        int nextQueueCount = 0;

        bool operator==(const StateKey& other) const;
    };

    struct StateKeyHash {
        std::size_t operator()(const StateKey& key) const;
    };

    struct TranspositionEntry {
        int visits = 0;
        double totalValue = 0.0;
    };

    using TranspositionTable = std::unordered_map<StateKey, TranspositionEntry, StateKeyHash>;

    double evalScoreDelta(const StepResult& r) const;
    double evalGreedyHeuristic(const tetris_env::BoardFeatures& before,
                               const tetris_env::BoardFeatures& after,
                               const StepResult& r) const;
    // Para GreedyHeuristic, os ponteiros de features devem ser validos (nao nulos).
    // Para ScoreDelta, os ponteiros sao ignorados e podem ser nulos.
    double stepValue(const StepResult& r,
                     const tetris_env::BoardFeatures* beforeFeatures,
                     const tetris_env::BoardFeatures* afterFeatures) const;
    SearchResult runSearch(const TetrisEnv& env,
                           const std::vector<Action>& rootActions,
                           int iterations,
                           std::mt19937& rng,
                           TranspositionTable* table);
    Action rolloutAction(const TetrisEnv& sim,
                         const std::vector<Action>& validActions,
                         std::mt19937& rng,
                         GreedyAgent& greedyPolicy) const;
    StateKey makeKey(const TetrisEnv& env) const;

    MctsParams params_;
    std::mt19937 rng_;
    std::size_t ttMaxEntries_ = 0;
    TranspositionTable transpositionTable_;
};
