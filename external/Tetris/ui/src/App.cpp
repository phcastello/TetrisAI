#include "tetris/App.hpp"

#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <ctime>
#include <algorithm>

#include "tetris_env/GreedyAgent.hpp"
#include "tetris_env/MctsConfig.hpp"
#include "tetris_env/RandomAgent.hpp"
#include "tetris_env/RunLogging.hpp"
#include "tetris_env/TetrisEnv.hpp"
#include "tetris_env/MctsRolloutAgent.hpp"

namespace tetris {

namespace {

std::string makeSafeSuffix(const std::string& name) {
    std::string safe;
    safe.reserve(name.size());
    for (unsigned char c : name) {
        if (std::isalnum(c) != 0) {
            safe.push_back(static_cast<char>(c));
        } else {
            safe.push_back('_');
        }
    }
    if (safe.empty()) {
        safe = "agent";
    }
    return safe;
}

std::string filenameSuffixForMode(PlayMode mode, const std::string& agentName) {
    if (mode == PlayMode::RandomAI) {
        return "_random";
    }
    if (mode == PlayMode::GreedyAI) {
        return "_greedy";
    }
    return "_" + makeSafeSuffix(agentName);
}

bool isMctsMode(PlayMode mode) {
    return mode == PlayMode::MctsGreedyAI ||
           mode == PlayMode::MctsDefaultAI ||
           mode == PlayMode::MctsTranspositionAI;
}

} // namespace


App::App() = default;

int App::run() {
    if (!initialize()) {
        return 1;
    }

    while (window_.isOpen()) {
        PlayMode selectedMode = mode_;
        if (!hud_->showMenu(window_, layout_, selectedMode)) {
            return 0;
        }
        mode_ = selectedMode;

        EpisodeReport report{};
        report.episodeIndex = 1;
        report.agentConfig.clear();

        bool episodePlayed = false;

        if (mode_ == PlayMode::Human) {
            report.agentName = "Human";
            report.modeName = "Human";

            game_.start();
            music_.play();
            episodePlayed = runGameLoopHuman(report);
            music_.stop();
        } else {
            std::string agentDir = "random_agent";
            switch (mode_) {
                case PlayMode::RandomAI:
                    report.agentName = "RandomAgent";
                    report.modeName = "RandomAI";
                    agentDir = "random_agent";
                    report.agentConfig.clear();
                    break;
                case PlayMode::GreedyAI:
                    report.agentName = "GreedyAgent";
                    report.modeName = "GreedyAI";
                    agentDir = "heuristic_greedy";
                    report.agentConfig.clear();
                    break;
                case PlayMode::MctsGreedyAI:
                    report.agentName = "mcts_greedy";
                    report.modeName = "MctsGreedyAI";
                    agentDir = "mcts_rollout";
                    report.agentConfig.clear();
                    break;
                case PlayMode::MctsDefaultAI:
                    report.agentName = "mcts_default";
                    report.modeName = "MctsDefaultAI";
                    agentDir = "mcts_rollout";
                    report.agentConfig.clear();
                    break;
                case PlayMode::MctsTranspositionAI:
                    report.agentName = "mcts_transposition";
                    report.modeName = "MctsTranspositionAI";
                    agentDir = "mcts_rollout";
                    report.agentConfig.clear();
                    break;
                default:
                    report.agentName = "Unknown";
                    report.modeName = "Unknown";
                    break;
            }
            report.runId = makeRunIdTimestamp();

            music_.play();
            episodePlayed = runGameLoopAI(mode_, report);
            music_.stop();

            if (episodePlayed && window_.isOpen()) {
                appendEpisodeReportToRunFile(report, agentDir, filenameSuffixForMode(mode_, report.agentName));
            }
        }

        if (!episodePlayed || !window_.isOpen()) {
            break;
        }

        if (!hud_->showGameOver(window_, layout_, report)) {
            break;
        }
    }

    return 0;
}

bool App::initialize() {
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    layout_ = config::makeLayout(desktop);

    window_.create(desktop, "Tetris", sf::Style::Fullscreen);
    window_.setVerticalSyncEnabled(true);

    const std::array<const char*, 3> fontPaths{
        config::fontPath,
        config::fontPathAlt,
        "../external/Tetris/assets/HennyPenny.ttf"};

    const char* loadedFontPath = nullptr;
    for (const auto* path : fontPaths) {
        if (font_.loadFromFile(path)) {
            loadedFontPath = path;
            break;
        }
    }
    if (!loadedFontPath) {
        std::cerr << "Fonte nao encontrada." << std::endl;
        return false;
    }

    const std::array<const char*, 3> musicPaths{
        config::musicPath,
        config::musicPathAlt,
        "../external/Tetris/assets/TetrisGameMusic.ogg"};

    bool musicLoaded = false;
    for (const auto* path : musicPaths) {
        if (music_.openFromFile(path)) {
            musicLoaded = true;
            break;
        }
    }
    if (!musicLoaded) {
        std::cerr << "Erro ao carregar a musica." << std::endl;
        return false;
    }
    music_.setLoop(true);
    music_.setVolume(15.0f);

    renderer_ = std::make_unique<Renderer>(layout_);
    hud_ = std::make_unique<Hud>(font_);

    return true;
}

bool App::runGameLoopHuman(EpisodeReport& report) {
    leftHeld_ = false;
    rightHeld_ = false;
    softDrop_ = false;
    leftTimer_ = 0.0f;
    rightTimer_ = 0.0f;

    sf::Clock clock;
    float elapsedSeconds = 0.0f;

    while (window_.isOpen() && game_.state() == GameState::Playing) {
        const float dt = clock.restart().asSeconds();
        elapsedSeconds += dt;

        sf::Event event{};
        while (window_.pollEvent(event)) {
            if (!handleEvent(event)) {
                return false;
            }
        }

        if (game_.state() != GameState::Playing) {
            break;
        }

        updateMovement(dt);
        game_.update(dt, softDrop_);

        renderer_->draw(window_, game_, font_);

        if (game_.state() == GameState::GameOver) {
            break;
        }
    }

    if (window_.isOpen()) {
        report.score = game_.score();
        report.elapsedSeconds = elapsedSeconds;
    }

    return window_.isOpen();
}

bool App::runGameLoopAI(PlayMode mode, EpisodeReport& report) {
    ::TetrisEnv env{};
    env.reset();

    std::optional<int> scoreLimit{};
    std::optional<double> timeLimitSeconds{};
    std::string endReason = "game_over";

    std::unique_ptr<::Agent> agent{};
    switch (mode) {
        case PlayMode::RandomAI:
            agent = std::make_unique<::RandomAgent>();
            break;
        case PlayMode::GreedyAI:
            agent = std::make_unique<::GreedyAgent>();
            report.agentConfig.clear();
            break;
        case PlayMode::MctsGreedyAI:
        case PlayMode::MctsDefaultAI:
        case PlayMode::MctsTranspositionAI: {
            ::MctsParams params{};
            std::string agentDir = "mcts_rollout";

            const auto configPath = findMctsConfigPath(agentDir);
            if (!configPath.has_value()) {
                std::cerr << "Erro: arquivo de configuracao do MCTS nao encontrado (procure agents/"
                          << agentDir << "/config.yaml).\n";
                return false;
            }
            if (!loadMctsParamsFromYaml(*configPath, params)) {
                return false;
            }

            if (mode == PlayMode::MctsDefaultAI) {
                params.rolloutPolicy = MctsRolloutPolicy::Random;
                params.useTranspositionTable = false;
            } else if (mode == PlayMode::MctsTranspositionAI) {
                params.rolloutPolicy = MctsRolloutPolicy::Greedy;
                params.useTranspositionTable = true;
            } else {
                params.rolloutPolicy = MctsRolloutPolicy::Greedy;
                params.useTranspositionTable = false;
            }

            std::cerr << "Config MCTS carregada de " << *configPath << '\n';
            report.agentConfig = buildMctsConfigString(params);
            scoreLimit = params.scoreLimit;
            timeLimitSeconds = params.timeLimitSeconds;

            agent = std::make_unique<::MctsRolloutAgent>(params);
            break;
        }
        case PlayMode::Human:
            // Human mode should not route through the AI loop.
            return false;
        default:
            return false;
    }

    sf::Clock clock;
    float accumulator = 0.0f;
    const float actionInterval = 0.4f;
    const bool mctsSelected = isMctsMode(mode);
    float elapsedSeconds = 0.0f;

    std::future<::Action> mctsFuture;
    bool mctsActionPending = false;

    auto launchMctsSearch = [&](const ::TetrisEnv& state) {
        if (!mctsSelected || mctsActionPending || state.isGameOver()) {
            return;
        }
        ::TetrisEnv snapshot = state.clone();
        mctsFuture = std::async(std::launch::async, [agentPtr = agent.get(), snapshot]() mutable {
            return agentPtr->chooseAction(snapshot);
        });
        mctsActionPending = true;
    };

    auto pollMctsAction = [&]() -> std::optional<::Action> {
        if (!mctsActionPending) {
            return std::nullopt;
        }
        if (mctsFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
            return std::nullopt;
        }
        mctsActionPending = false;
        return mctsFuture.get();
    };

    if (mctsSelected) {
        launchMctsSearch(env);
    }

    bool shouldStop = false;
    while (window_.isOpen() && !env.isGameOver() && !shouldStop) {
        const float dt = clock.restart().asSeconds();
        accumulator += dt;
        accumulator = std::min(accumulator, 5.0f);
        elapsedSeconds += dt;

        sf::Event event{};
        while (window_.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window_.close();
                return false;
            }
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                window_.close();
                return false;
            }
        }

        if (mctsSelected) {
            if (scoreLimit.has_value() && env.getScore() >= *scoreLimit) {
                shouldStop = true;
                endReason = "score_limit";
            } else if (timeLimitSeconds.has_value() && elapsedSeconds >= *timeLimitSeconds) {
                shouldStop = true;
                endReason = "time_limit";
            }
        }

        if (shouldStop) {
            break;
        }

        if (mctsSelected && !mctsActionPending && !env.isGameOver()) {
            launchMctsSearch(env);
        }

        if (accumulator >= actionInterval && !env.isGameOver()) {
            const auto actions = env.getValidActions();
            if (actions.empty()) {
                shouldStop = true;
            } else {
                bool actionReady = true;
                ::Action action{};

                if (mctsSelected) {
                    auto readyAction = pollMctsAction();
                    if (!readyAction.has_value()) {
                        actionReady = false;
                    } else {
                        action = *readyAction;
                    }
                } else {
                    action = agent->chooseAction(env);
                }

                if (actionReady) {
                    accumulator -= actionInterval;
                    const auto result = env.step(action);
                    if (result.done) {
                        shouldStop = true;
                    }
                    if (mctsSelected && !shouldStop) {
                        launchMctsSearch(env);
                    }
                }
            }
        }

        renderer_->draw(window_, env.game(), font_);
    }

    if (mctsSelected && mctsActionPending) {
        mctsFuture.wait();
    }

    report.score = env.getScore();
    report.totalLines = env.getTotalLinesCleared();
    report.totalTurns = env.getTurnNumber();
    report.holdsUsed = env.getHoldsUsed();
    report.elapsedSeconds = elapsedSeconds;
    report.endReason = endReason;

    return window_.isOpen();
}

bool App::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::Closed) {
        window_.close();
        return false;
    }

    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Escape:
                window_.close();
                return false;
            case sf::Keyboard::Left:
                game_.moveLeft();
                leftHeld_ = true;
                leftTimer_ = 0.0f;
                break;
            case sf::Keyboard::Right:
                game_.moveRight();
                rightHeld_ = true;
                rightTimer_ = 0.0f;
                break;
            case sf::Keyboard::Down:
                softDrop_ = true;
                break;
            case sf::Keyboard::Up:
                game_.rotate();
                break;
            case sf::Keyboard::Space:
                game_.hardDrop();
                break;
            case sf::Keyboard::C:
                game_.hold();
                break;
            default:
                break;
        }
    }

    if (event.type == sf::Event::KeyReleased) {
        switch (event.key.code) {
            case sf::Keyboard::Left:
                leftHeld_ = false;
                leftTimer_ = 0.0f;
                break;
            case sf::Keyboard::Right:
                rightHeld_ = false;
                rightTimer_ = 0.0f;
                break;
            case sf::Keyboard::Down:
                softDrop_ = false;
                break;
            default:
                break;
        }
    }

    return true;
}

void App::updateMovement(float dt) {
    if (leftHeld_) {
        leftTimer_ += dt;
        if (leftTimer_ >= engine_cfg::moveRepeatDelay) {
            game_.moveLeft();
            leftTimer_ = 0.0f;
        }
    }

    if (rightHeld_) {
        rightTimer_ += dt;
        if (rightTimer_ >= engine_cfg::moveRepeatDelay) {
            game_.moveRight();
            rightTimer_ = 0.0f;
        }
    }
}

} // namespace tetris
