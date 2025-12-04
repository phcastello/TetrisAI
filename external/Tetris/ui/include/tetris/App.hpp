#pragma once

#include <memory>

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>

#include "tetris/Config.hpp"
#include "tetris/EpisodeReport.hpp"
#include "tetris/Game.hpp"
#include "tetris/Hud.hpp"
#include "tetris/PlayMode.hpp"
#include "tetris/Renderer.hpp"

namespace tetris {

class App {
public:
    App();

    int run();

private:
    bool initialize();
    bool runGameLoopHuman(EpisodeReport& report);
    bool runGameLoopAI(PlayMode mode, EpisodeReport& report);
    bool handleEvent(const sf::Event& event);
    void updateMovement(float dt);

    sf::RenderWindow window_{};
    sf::Font font_{};
    sf::Music music_{};
    config::Layout layout_{};
    Game game_{};
    std::unique_ptr<Renderer> renderer_{};
    std::unique_ptr<Hud> hud_{};

    bool leftHeld_ = false;
    bool rightHeld_ = false;
    float leftTimer_ = 0.0f;
    float rightTimer_ = 0.0f;
    bool softDrop_ = false;
    PlayMode mode_ = PlayMode::Human;
};

} // namespace tetris
