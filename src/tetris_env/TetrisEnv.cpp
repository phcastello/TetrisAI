#include "tetris_env/TetrisEnv.hpp"

#include <algorithm>

TetrisEnv::TetrisEnv() {
    reset();
}

void TetrisEnv::reset() {
    game_.start();
    totalLinesCleared_ = 0;
    turnNumber_ = 0;
    holdsUsed_ = 0;
}

StepResult TetrisEnv::step(const Action& action) {
    if (isGameOver()) {
        return StepResult{0, 0, 0, true};
    }

    const int previousScore = game_.score();
    bool usedHold = false;

    if (action.useHold && game_.canHold()) {
        game_.hold();
        usedHold = true;
    }

    if (isGameOver() || !game_.hasActivePiece()) {
        return StepResult{0, 0, game_.score() - previousScore, true};
    }

    const auto placement = game_.placeActive(action.rotation, action.targetX);
    const int scoreDelta = placement.scoreDelta;
    if (!placement.success) {
        return StepResult{0, 0, scoreDelta, true};
    }
    if (usedHold) {
        ++holdsUsed_;
    }

    if (placement.linesCleared > 0) {
        totalLinesCleared_ += placement.linesCleared;
    }
    ++turnNumber_;

    const bool done = isGameOver();
    return StepResult{placement.linesCleared, placement.linesCleared, scoreDelta, done};
}

TetrisEnv TetrisEnv::clone() const {
    return *this;
}

bool TetrisEnv::isGameOver() const {
    return game_.state() == tetris::GameState::GameOver;
}

const tetris::Game& TetrisEnv::game() const {
    return game_;
}

int TetrisEnv::getScore() const {
    return game_.score();
}

int TetrisEnv::getTotalLinesCleared() const {
    return totalLinesCleared_;
}

int TetrisEnv::getTurnNumber() const {
    return turnNumber_;
}

int TetrisEnv::getHoldsUsed() const {
    return holdsUsed_;
}

const tetris::Board& TetrisEnv::getBoard() const {
    return game_.board();
}

tetris_env::PieceType TetrisEnv::getCurrentPieceType() const {
    return game_.activePiece().id;
}

int TetrisEnv::getCurrentPieceRotation() const {
    return game_.activePiece().rotation;
}

int TetrisEnv::getCurrentPieceX() const {
    return game_.activePiece().origin.x;
}

int TetrisEnv::getCurrentPieceY() const {
    return game_.activePiece().origin.y;
}

std::optional<tetris_env::PieceType> TetrisEnv::getHoldPieceType() const {
    if (!game_.hasHoldPiece()) {
        return std::nullopt;
    }
    return game_.holdPiece();
}

std::vector<tetris_env::PieceType> TetrisEnv::getNextQueue() const {
    return game_.queuePreview(queueSize_);
}

int TetrisEnv::getBoardWidth() const {
    return tetris::engine_cfg::fieldWidth;
}

int TetrisEnv::getBoardHeight() const {
    return tetris::engine_cfg::fieldHeight;
}

std::vector<Action> TetrisEnv::getValidActions() const {
    std::vector<Action> actions;
    if (isGameOver() || !game_.hasActivePiece()) {
        return actions;
    }

    generateActionsForPiece(game_.activePiece(), false, actions);

    if (game_.canHold()) {
        tetris::ActivePiece holdPiece{};
        if (game_.hasHoldPiece()) {
            holdPiece.id = game_.holdPiece();
        } else {
            auto preview = game_.queuePreview(1);
            if (preview.empty()) {
                return actions;
            }
            holdPiece.id = preview.front();
        }

        holdPiece.rotation = 0;
        holdPiece.origin = tetris::Game::spawnOrigin();
        generateActionsForPiece(holdPiece, true, actions);
    }

    return actions;
}

void TetrisEnv::generateActionsForPiece(const tetris::ActivePiece& piece, bool useHold, std::vector<Action>& actions) const {
    if (piece.id < 0) {
        return;
    }

    if (!game_.canPlace(piece.id, piece.rotation, piece.origin)) {
        return;
    }

    for (int rotation = 0; rotation < 4; ++rotation) {
        const auto offsets = game_.computeCells(piece.id, rotation, tetris::Cell{0, 0});
        int minOffset = offsets[0].x;
        int maxOffset = offsets[0].x;
        for (const auto& cell : offsets) {
            minOffset = std::min(minOffset, cell.x);
            maxOffset = std::max(maxOffset, cell.x);
        }

        const int minX = -minOffset;
        const int maxX = tetris::engine_cfg::fieldWidth - 1 - maxOffset;

        for (int targetX = minX; targetX <= maxX; ++targetX) {
            tetris::ActivePiece landing{};
            if (game_.simulatePlacement(piece, rotation, targetX, landing)) {
                actions.push_back(Action{rotation, landing.origin.x, useHold});
            }
        }
    }
}
