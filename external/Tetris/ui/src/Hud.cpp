#include "tetris/Hud.hpp"

#include <iomanip>
#include <sstream>
#include <string>

namespace tetris {

Hud::Hud(const sf::Font& font) : font_(font) {}

bool Hud::showMenu(sf::RenderWindow& window, const config::Layout& layout, PlayMode& mode) {
    PlayMode currentMode = mode;

    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return false;
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    return false;
                }
                if (event.key.code == sf::Keyboard::Enter) {
                    mode = currentMode;
                    return true;
                }
                if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::Numpad1) {
                    currentMode = PlayMode::Human;
                }
                if (event.key.code == sf::Keyboard::Num2 || event.key.code == sf::Keyboard::Numpad2) {
                    currentMode = PlayMode::RandomAI;
                }
                if (event.key.code == sf::Keyboard::Num3 || event.key.code == sf::Keyboard::Numpad3) {
                    currentMode = PlayMode::GreedyAI;
                }
                if (event.key.code == sf::Keyboard::Num4 || event.key.code == sf::Keyboard::Numpad4) {
                    currentMode = PlayMode::MctsGreedyAI;
                }
                if (event.key.code == sf::Keyboard::Num5 || event.key.code == sf::Keyboard::Numpad5) {
                    currentMode = PlayMode::MctsDefaultAI;
                }
                if (event.key.code == sf::Keyboard::Num6 || event.key.code == sf::Keyboard::Numpad6) {
                    currentMode = PlayMode::MctsTranspositionAI;
                }
            }
        }

        window.clear(sf::Color::Black);

        sf::Text title("TETRIS", font_, 80);
        title.setFillColor(sf::Color::White);
        const auto titleBounds = title.getLocalBounds();
        title.setPosition(
            layout.desktop.width / 2.0f - titleBounds.width / 2.0f,
            layout.desktop.height * 0.2f);
        window.draw(title);

        sf::Text play("Pressione ENTER para Jogar", font_, 40);
        play.setFillColor(sf::Color::Green);
        const auto playBounds = play.getLocalBounds();
        play.setPosition(
            layout.desktop.width / 2.0f - playBounds.width / 2.0f,
            layout.desktop.height * 0.78f);
        window.draw(play);

        sf::Text exit("Pressione ESC para Sair", font_, 40);
        exit.setFillColor(sf::Color::Red);
        const auto exitBounds = exit.getLocalBounds();
        exit.setPosition(
            layout.desktop.width / 2.0f - exitBounds.width / 2.0f,
            layout.desktop.height * 0.84f);
        window.draw(exit);

        sf::Text signature("Criadores: Pedro Hasson Castello, Ruan Pablo Martins, Patrick Correa", font_, 28);
        signature.setFillColor(sf::Color(180, 180, 180));
        const auto signatureBounds = signature.getLocalBounds();
        signature.setPosition(
            layout.desktop.width / 2.0f - signatureBounds.width / 2.0f,
            layout.desktop.height * 0.9f);
        window.draw(signature);

        sf::Text modesTitle("Escolha o modo:", font_, 40);
        modesTitle.setFillColor(sf::Color::White);
        const auto modesBounds = modesTitle.getLocalBounds();
        modesTitle.setPosition(
            layout.desktop.width / 2.0f - modesBounds.width / 2.0f,
            layout.desktop.height * 0.36f);
        window.draw(modesTitle);

        const auto drawOption = [&](const std::string& text, float y, bool selected) {
            sf::Text option(text, font_, 34);
            option.setFillColor(selected ? sf::Color::Green : sf::Color::White);
            const auto bounds = option.getLocalBounds();
            option.setPosition(
                layout.desktop.width / 2.0f - bounds.width / 2.0f,
                y);
            window.draw(option);
        };

        drawOption("1 - Humano (Teclado)", layout.desktop.height * 0.42f, currentMode == PlayMode::Human);
        drawOption("2 - Random AI", layout.desktop.height * 0.46f, currentMode == PlayMode::RandomAI);
        drawOption("3 - Greedy AI", layout.desktop.height * 0.50f, currentMode == PlayMode::GreedyAI);
        drawOption("4 - MCTS Greedy", layout.desktop.height * 0.54f, currentMode == PlayMode::MctsGreedyAI);
        drawOption("5 - MCTS Default (rollout aleatorio)", layout.desktop.height * 0.58f, currentMode == PlayMode::MctsDefaultAI);
        drawOption("6 - MCTS com tabela de transposicao", layout.desktop.height * 0.62f, currentMode == PlayMode::MctsTranspositionAI);

        window.display();
    }

    return false;
}

bool Hud::showGameOver(sf::RenderWindow& window, const config::Layout& layout, const EpisodeReport& report) {
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return false;
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    return false;
                }
                if (event.key.code == sf::Keyboard::Enter) {
                    return true;
                }
            }
        }

        window.clear(sf::Color::Black);

        sf::Text gameOver("GAME OVER", font_, 80);
        gameOver.setFillColor(sf::Color::Red);
        const auto overBounds = gameOver.getLocalBounds();
        gameOver.setPosition(
            layout.desktop.width / 2.0f - overBounds.width / 2.0f,
            layout.desktop.height * 0.3f);
        window.draw(gameOver);

        sf::Text scoreText("Score: " + std::to_string(report.score), font_, 70);
        scoreText.setFillColor(sf::Color::Yellow);
        const auto scoreBounds = scoreText.getLocalBounds();
        scoreText.setPosition(
            layout.desktop.width / 2.0f - scoreBounds.width / 2.0f,
            layout.desktop.height * 0.4f);
        window.draw(scoreText);

        const bool isHuman = report.agentName == "Human";
        float nextLineY = layout.desktop.height * 0.5f;
        if (!isHuman) {
            sf::Text linesText("Linhas: " + std::to_string(report.totalLines), font_, 40);
            linesText.setFillColor(sf::Color::White);
            const auto linesBounds = linesText.getLocalBounds();
            linesText.setPosition(
                layout.desktop.width / 2.0f - linesBounds.width / 2.0f,
                nextLineY);
            window.draw(linesText);
            nextLineY += linesBounds.height + 10.0f;

            sf::Text turnsText("Pecas: " + std::to_string(report.totalTurns), font_, 40);
            turnsText.setFillColor(sf::Color::White);
            const auto turnsBounds = turnsText.getLocalBounds();
            turnsText.setPosition(
                layout.desktop.width / 2.0f - turnsBounds.width / 2.0f,
                nextLineY);
            window.draw(turnsText);
            nextLineY += turnsBounds.height + 10.0f;

            sf::Text holdsText("Holds: " + std::to_string(report.holdsUsed), font_, 40);
            holdsText.setFillColor(sf::Color::White);
            const auto holdsBounds = holdsText.getLocalBounds();
            holdsText.setPosition(
                layout.desktop.width / 2.0f - holdsBounds.width / 2.0f,
                nextLineY);
            window.draw(holdsText);
            nextLineY += holdsBounds.height + 10.0f;

            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << report.elapsedSeconds;
            sf::Text timeText("Tempo: " + ss.str() + " s", font_, 40);
            timeText.setFillColor(sf::Color::White);
            const auto timeBounds = timeText.getLocalBounds();
            timeText.setPosition(
                layout.desktop.width / 2.0f - timeBounds.width / 2.0f,
                nextLineY);
            window.draw(timeText);
            nextLineY += timeBounds.height + 20.0f;

            if (!report.agentConfig.empty()) {
                sf::Text configText("Config: " + report.agentConfig, font_, 32);
                configText.setFillColor(sf::Color(200, 200, 200));
                const auto configBounds = configText.getLocalBounds();
                configText.setPosition(
                    layout.desktop.width / 2.0f - configBounds.width / 2.0f,
                    nextLineY);
                window.draw(configText);
                nextLineY += configBounds.height + 20.0f;
            }
        } else {
            nextLineY = layout.desktop.height * 0.55f;
        }

        sf::Text restart("ENTER - Voltar ao menu", font_, 40);
        restart.setFillColor(sf::Color::White);
        const auto restartBounds = restart.getLocalBounds();
        restart.setPosition(
            layout.desktop.width / 2.0f - restartBounds.width / 2.0f,
            nextLineY);
        window.draw(restart);
        nextLineY += restartBounds.height + 10.0f;

        sf::Text exit("ESC - Sair", font_, 40);
        exit.setFillColor(sf::Color::White);
        const auto exitBounds = exit.getLocalBounds();
        exit.setPosition(
            layout.desktop.width / 2.0f - exitBounds.width / 2.0f,
            nextLineY);
        window.draw(exit);

        sf::Text signature("Criadores: Pedro Hasson Castello, Ruan Pablo Martins, Patrick Correa", font_, 28);
        signature.setFillColor(sf::Color(180, 180, 180));
        const auto signatureBounds = signature.getLocalBounds();
        signature.setPosition(
            layout.desktop.width / 2.0f - signatureBounds.width / 2.0f,
            layout.desktop.height * 0.82f);
        window.draw(signature);

        window.display();
    }

    return false;
}

} // namespace tetris
