#include "tetris_env/TetrisEnv.hpp"

#include <algorithm>
#include <utility>

namespace {

tetris::Cell computeSpawnOrigin() {
    return tetris::Cell{tetris::engine_cfg::fieldWidth / 2 - 2, 0};
}

} // namespace

TetrisEnv::TetrisEnv() {
    reset();
}

void TetrisEnv::reset() {
    board_.clear();
    score_.reset();
    totalLinesCleared_ = 0;
    turnNumber_ = 0;
    hold_ = -1;
    holdUsed_ = false;
    state_ = tetris::GameState::Playing;

    while (!nextPieces_.empty()) {
        nextPieces_.pop();
    }

    bag_.resetHistory();
    bag_.refill(nextPieces_, queueSize_);
    active_ = tetris::ActivePiece{};
    spawnFromQueue();
}

StepResult TetrisEnv::step(const Action& action) {
    if (state_ == tetris::GameState::GameOver) {
        return StepResult{0, 0, 0, true};
    }

    const int previousScore = score_.value;

    if (action.useHold && !holdUsed_) {
        applyHold();
    }

    if (state_ == tetris::GameState::GameOver || active_.id < 0) {
        return StepResult{0, 0, score_.value - previousScore, true};
    }

    const int desiredRotation = normalizeRotation(action.rotation);

    tetris::ActivePiece landing{};
    const bool placed = simulatePlacement(active_, desiredRotation, action.targetX, landing);
    if (!placed) {
        state_ = tetris::GameState::GameOver;
        return StepResult{0, 0, score_.value - previousScore, true};
    }

    active_ = landing;
    const auto cells = computeCells(active_.id, active_.rotation, active_.origin);
    board_.lock(cells, active_.id + 1);

    const int cleared = board_.clearFullLines();
    if (cleared > 0) {
        score_.addLines(cleared);
        totalLinesCleared_ += cleared;
    }
    ++turnNumber_;

    spawnFromQueue();
    const bool done = state_ == tetris::GameState::GameOver;

    const int scoreDelta = score_.value - previousScore;
    // reward espelha o número de linhas limpas por simplicidade inicial.
    return StepResult{cleared, cleared, scoreDelta, done};
}

TetrisEnv TetrisEnv::clone() const {
    return *this;
}

bool TetrisEnv::isGameOver() const {
    return state_ == tetris::GameState::GameOver;
}

int TetrisEnv::getScore() const {
    return score_.value;
}

int TetrisEnv::getTotalLinesCleared() const {
    return totalLinesCleared_;
}

int TetrisEnv::getTurnNumber() const {
    return turnNumber_;
}

const tetris::Board& TetrisEnv::getBoard() const {
    return board_;
}

tetris_env::PieceType TetrisEnv::getCurrentPieceType() const {
    return active_.id;
}

int TetrisEnv::getCurrentPieceRotation() const {
    return active_.rotation;
}

int TetrisEnv::getCurrentPieceX() const {
    return active_.origin.x;
}

int TetrisEnv::getCurrentPieceY() const {
    return active_.origin.y;
}

std::optional<tetris_env::PieceType> TetrisEnv::getHoldPieceType() const {
    if (hold_ == -1) {
        return std::nullopt;
    }
    return hold_;
}

std::vector<tetris_env::PieceType> TetrisEnv::getNextQueue() const {
    return bag_.peekN(nextPieces_, queueSize_);
}

int TetrisEnv::getBoardWidth() const {
    return tetris::engine_cfg::fieldWidth;
}

int TetrisEnv::getBoardHeight() const {
    return tetris::engine_cfg::fieldHeight;
}

std::vector<Action> TetrisEnv::getValidActions() const {
    std::vector<Action> actions;
    if (state_ == tetris::GameState::GameOver || active_.id < 0) {
        return actions;
    }

    generateActionsForPiece(active_, false, actions);

    if (!holdUsed_) {
        tetris::ActivePiece holdPiece{};
        bool canUseHold = false;

        if (hold_ == -1) {
            if (!nextPieces_.empty()) {
                holdPiece.id = nextPieces_.front();
                canUseHold = true;
            }
        } else {
            holdPiece.id = hold_;
            canUseHold = true;
        }

        if (canUseHold) {
            holdPiece.rotation = 0;
            holdPiece.origin = spawnOrigin();
            generateActionsForPiece(holdPiece, true, actions);
        }
    }

    return actions;
}

tetris::Cell TetrisEnv::spawnOrigin() {
    return computeSpawnOrigin();
}

int TetrisEnv::normalizeRotation(int rotation) {
    int normalized = rotation % 4;
    if (normalized < 0) {
        normalized += 4;
    }
    return normalized;
}

bool TetrisEnv::canPlace(int pieceId, int rotation, const tetris::Cell& origin) const {
    const auto cells = computeCells(pieceId, rotation, origin);
    return board_.canPlace(cells);
}

std::array<tetris::Cell, 4> TetrisEnv::computeCells(int pieceId, int rotation, const tetris::Cell& origin) const {
    return tetris::TetrominoSet::instance().cells(pieceId, rotation, origin);
}

void TetrisEnv::spawnFromQueue() {
    bag_.refill(nextPieces_, queueSize_);
    if (nextPieces_.empty()) {
        state_ = tetris::GameState::GameOver;
        active_.id = -1;
        return;
    }

    active_.id = nextPieces_.front();
    nextPieces_.pop();
    bag_.refill(nextPieces_, queueSize_);

    active_.rotation = 0;
    active_.origin = spawnOrigin();
    holdUsed_ = false;
    bag_.registerUse(active_.id);
    if (!canPlace(active_.id, active_.rotation, active_.origin)) {
        state_ = tetris::GameState::GameOver;
    } else {
        state_ = tetris::GameState::Playing;
    }
}

void TetrisEnv::applyHold() {
    if (state_ != tetris::GameState::Playing || active_.id < 0 || holdUsed_) {
        return;
    }

    if (hold_ == -1) {
        hold_ = active_.id;
        spawnFromQueue();
    } else {
        std::swap(hold_, active_.id);
        active_.rotation = 0;
        active_.origin = spawnOrigin();
        bag_.registerUse(active_.id);
        if (!canPlace(active_.id, active_.rotation, active_.origin)) {
            state_ = tetris::GameState::GameOver;
        }
    }

    holdUsed_ = true;
}

bool TetrisEnv::simulatePlacement(const tetris::ActivePiece& startPiece, int targetRotation, int targetX,
                                  tetris::ActivePiece& landing) const {
    if (startPiece.id < 0) {
        return false;
    }

    const int normalizedRotation = normalizeRotation(targetRotation);
    tetris::ActivePiece piece = startPiece;

    if (!canPlace(piece.id, piece.rotation, piece.origin)) {
        return false;
    }

    while (piece.rotation != normalizedRotation) {
        const int nextRotation = (piece.rotation + 1) % 4;
        if (canPlace(piece.id, nextRotation, piece.origin)) {
            piece.rotation = nextRotation;
        } else {
            return false;
        }
    }

    int dx = targetX - piece.origin.x;
    const int step = (dx > 0) ? 1 : -1;
    while (dx != 0) {
        const tetris::Cell nextOrigin{piece.origin.x + step, piece.origin.y};
        if (canPlace(piece.id, piece.rotation, nextOrigin)) {
            piece.origin = nextOrigin;
            dx -= step;
        } else {
            return false;
        }
    }

    while (canPlace(piece.id, piece.rotation, tetris::Cell{piece.origin.x, piece.origin.y + 1})) {
        ++piece.origin.y;
    }

    landing = piece;
    return true;
}

void TetrisEnv::generateActionsForPiece(const tetris::ActivePiece& piece, bool useHold, std::vector<Action>& actions) const {
    if (piece.id < 0) {
        return;
    }

    if (!canPlace(piece.id, piece.rotation, piece.origin)) {
        return;
    }

    for (int rotation = 0; rotation < 4; ++rotation) {
        const auto offsets = computeCells(piece.id, rotation, tetris::Cell{0, 0});
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
            if (simulatePlacement(piece, rotation, targetX, landing)) {
                actions.push_back(Action{rotation, landing.origin.x, useHold});
            }
        }
    }
}
