#pragma once

#include <optional>
#include <vector>

#include "Action.hpp"
#include "StepResult.hpp"
#include "Types.hpp"
#include "tetris/Game.hpp"

class TetrisEnv {
public:
    TetrisEnv();

    void reset();
    StepResult step(const Action& action);
    TetrisEnv clone() const;

    bool isGameOver() const;
    const tetris::Game& game() const;

    int getScore() const;
    int getTotalLinesCleared() const;
    int getTurnNumber() const;
    int getHoldsUsed() const;

    const tetris::Board& getBoard() const;
    tetris_env::PieceType getCurrentPieceType() const;
    int getCurrentPieceRotation() const;
    int getCurrentPieceX() const;
    int getCurrentPieceY() const;

    std::optional<tetris_env::PieceType> getHoldPieceType() const;
    std::vector<tetris_env::PieceType> getNextQueue() const;

    int getBoardWidth() const;
    int getBoardHeight() const;

    std::vector<Action> getValidActions() const;

private:
    void generateActionsForPiece(const tetris::ActivePiece& piece, bool useHold, std::vector<Action>& actions) const;

    tetris::Game game_{};
    int totalLinesCleared_ = 0;
    int turnNumber_ = 0;
    int holdsUsed_ = 0;
    std::size_t queueSize_ = static_cast<std::size_t>(tetris::engine_cfg::queuePreviewCount);
};
