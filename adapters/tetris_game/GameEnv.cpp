#include "GameEnv.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

// TODO: Conectar aos tipos reais do submódulo (tetris::Game, Board, RNG, etc.).
// Mapeamento esperado de ações:
//  - Action::Left / Action::Right: deslocamento lateral da peça ativa.
//  - Action::Rotate: rotação conforme regras do jogo (SRS atual).
//  - Action::SoftDrop: avanço discreto com recompensa pequena ou nula.
//  - Action::HardDrop: queda instantânea, recompensa baseada no delta de linhas/score.
//  - Action::Hold: troca com peça reserva quando permitido.
//  - Action::None: passo vazio, útil para estados terminais ou espera controlada.
// Recompensa por passo: delta da pontuação/linhas após a ação.

namespace tetrissdk::adapters {

struct GameEnv::Impl {
    // TODO: Guardar instâncias de tetris::Game, estado de RNG, pontuação acumulada etc.
};

GameEnv::GameEnv() : impl_(std::make_shared<Impl>()) {}
GameEnv::GameEnv(const GameEnv&) = default;
GameEnv::GameEnv(GameEnv&&) noexcept = default;
GameEnv& GameEnv::operator=(const GameEnv&) = default;
GameEnv& GameEnv::operator=(GameEnv&&) noexcept = default;
GameEnv::~GameEnv() = default;

void GameEnv::reset(std::uint64_t /*seed*/) {
    throw std::logic_error{"GameEnv::reset ainda não foi implementado"};
}

StepResult GameEnv::step(Action /*action*/) {
    throw std::logic_error{"GameEnv::step ainda não foi implementado"};
}

std::vector<Action> GameEnv::valid_actions() const {
    return {};
}

std::unique_ptr<Environment> GameEnv::clone() const {
    throw std::logic_error{"GameEnv::clone ainda não foi implementado"};
}

int GameEnv::get_score() const {
    return 0;
}

void GameEnv::serialize(std::vector<std::uint8_t>& buffer) const {
    buffer.clear();
}

}  // namespace tetrissdk::adapters
