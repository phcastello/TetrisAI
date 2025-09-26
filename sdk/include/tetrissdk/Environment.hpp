#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace tetrissdk {

// Representa as ações atômicas disponíveis para um agente.
enum class Action {
    Left,
    Right,
    Rotate,
    SoftDrop,
    HardDrop,
    Hold,
    None
};

// Resultado retornado por uma chamada a step().
struct StepResult {
    double reward = 0.0;   // Delta de recompensa causado pela ação executada.
    bool done = false;     // Verdadeiro quando o episódio terminou.
    int score = 0;         // Pontuação acumulada exposta pelo jogo.
};

// Contrato genérico para ambientes de Tetris consumidos por agentes.
class Environment {
public:
    virtual ~Environment() = default;

    // Reinicia o episódio com uma semente determinística para o gerador de peças.
    virtual void reset(std::uint64_t seed) = 0;

    // Executa exatamente uma ação e retorna o resultado da transição de estado.
    virtual StepResult step(Action action) = 0;

    // Lista de ações válidas no estado atual. Ações inválidas não devem aparecer.
    virtual std::vector<Action> valid_actions() const = 0;

    // Produz uma cópia independente do ambiente atual (inclusive RNG e pontuação).
    virtual std::unique_ptr<Environment> clone() const = 0;

    // Pontuação acumulada até o estado corrente, na mesma escala apresentada pela UI.
    virtual int get_score() const = 0;

    // Serialização compacta do estado para usos como tabelas de transposição.
    // Implementações podem optar por um "no-op" enquanto o formato não for definido.
    virtual void serialize(std::vector<std::uint8_t>& buffer) const = 0;
};

}  // namespace tetrissdk
