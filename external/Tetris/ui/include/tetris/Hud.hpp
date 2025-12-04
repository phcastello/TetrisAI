#pragma once

#include <SFML/Graphics.hpp>

#include "tetris/Config.hpp"
#include "tetris/PlayMode.hpp"
#include "tetris/EpisodeReport.hpp"

namespace tetris {

class Hud {
public:
    explicit Hud(const sf::Font& font);

    bool showMenu(sf::RenderWindow& window, const config::Layout& layout, PlayMode& mode);
    bool showGameOver(sf::RenderWindow& window, const config::Layout& layout, const EpisodeReport& report);

private:
    const sf::Font& font_;
};

} // namespace tetris
