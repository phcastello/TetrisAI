#pragma once

#include <array>
#include <optional>
#include <queue>
#include <vector>

#include "Action.hpp"
#include "StepResult.hpp"
#include "Types.hpp"
#include "tetris/Bag.hpp"
#include "tetris/Board.hpp"
#include "tetris/Tetromino.hpp"
#include "tetris/Types.hpp"

class TetrisEnv {
public:
    TetrisEnv();

    void reset();
    StepResult step(const Action& action);
    TetrisEnv clone() const;

    bool isGameOver() const;

    int getScore() const;
    int getTotalLinesCleared() const;
    int getTurnNumber() const;

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
    static tetris::Cell spawnOrigin();
    static int normalizeRotation(int rotation);

    bool canPlace(int pieceId, int rotation, const tetris::Cell& origin) const;
    std::array<tetris::Cell, 4> computeCells(int pieceId, int rotation, const tetris::Cell& origin) const;
    void spawnFromQueue();
    void applyHold();
    bool simulatePlacement(const tetris::ActivePiece& startPiece, int targetRotation, int targetX,
                           tetris::ActivePiece& landing) const;
    void generateActionsForPiece(const tetris::ActivePiece& piece, bool useHold, std::vector<Action>& actions) const;

    tetris::Board board_{};
    tetris::Bag bag_{};
    std::queue<int> nextPieces_{};
    tetris::ActivePiece active_{};
    int hold_ = -1;
    bool holdUsed_ = false;
    tetris::Score score_{};
    tetris::GameState state_ = tetris::GameState::Menu;
    int totalLinesCleared_ = 0;
    int turnNumber_ = 0;
    std::size_t queueSize_ = static_cast<std::size_t>(tetris::engine_cfg::queuePreviewCount);
};
