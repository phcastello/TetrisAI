#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "tetrissdk/Environment.hpp"

namespace tetris {
class Game;
}  // namespace tetris

namespace tetrissdk::adapters {

// Adaptador que conecta o contrato Environment ao Game do submódulo Tetris.
class GameEnv : public tetrissdk::Environment {
public:
    GameEnv();
    GameEnv(const GameEnv&);
    GameEnv(GameEnv&&) noexcept;
    GameEnv& operator=(const GameEnv&);
    GameEnv& operator=(GameEnv&&) noexcept;
    ~GameEnv();

    void reset(std::uint64_t seed) override;
    StepResult step(Action action) override;
    std::vector<Action> valid_actions() const override;
    std::unique_ptr<Environment> clone() const override;
    int get_score() const override;
    void serialize(std::vector<std::uint8_t>& buffer) const override;

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;  // TODO: encapsular estado completo de Game e RNG.
};

}  // namespace tetrissdk::adapters
