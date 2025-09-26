#include <iostream>

// TODO: Integrar com tetrissdk::adapters::GameEnv.
// Fluxo esperado:
//  1. Instanciar GameEnv e chamar reset(seed) com semente fixa.
//  2. Repetir: escolher ação (aleatória ou heurística), executar step(action) e acumular métricas.
//  3. Encerrar ao receber done = true e imprimir score, linhas, passos e tempos médios.
int main() {
    std::cout << "ai_runner stub\n";
    return 0;
}
