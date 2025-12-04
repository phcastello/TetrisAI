# TetrisAI

Ambiente de Tetris em C++20 voltado para experimentos de IA (Random, Greedy e MCTS Rollout). A biblioteca `tetris_env` expõe uma API simples para agentes e o projeto já inclui binários para rodar partidas de teste e execuções em batch sem GUI.

## Estrutura
- `include/` e `src/tetris_env/`: API e implementação do ambiente e agentes básicos.
- `apps/`: executáveis `tetris_env_test` (smoke test) e `tetris_batch_runner` (execução em lote).
- `config/batch_runs.yaml`: configuração padrão para o runner em batch.
- `agents/`: configs de referência e diretório onde os CSVs de resultados são gravados.
- `external/Tetris/`: motor de jogo (sempre usado) e UI opcional via SFML.

## Pré-requisitos
- CMake 3.20+.
- Compilador C++20 (g++/clang/MSVC).
- Opcional para GUI: SFML 2.5+ instalada (`-DBUILD_TETRIS_GUI=ON`).

## Como compilar
Linux/macOS (Release):
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Windows (MSVC, terminal x64):
```powershell
cmake -S . -B build
cmake --build build --config Release
```

- Alvos principais: `tetris_env_test`, `tetris_batch_runner` e, se habilitado, `tetris_gui`.
- Para Debug, troque `-DCMAKE_BUILD_TYPE=Debug` ou `--config Debug`.

## Rodando
- Teste rápido do ambiente com agente aleatório:
  - Linux/macOS: `./build/tetris_env_test`
  - Windows: `.\build\Release\tetris_env_test.exe`

- Execução em batch (usa `config/batch_runs.yaml` por padrão):
  ```bash
  ./build/tetris_batch_runner
  # ou
  ./build/tetris_batch_runner config/minha_config.yaml
  # ajuda rápida
  ./build/tetris_batch_runner --help
  ```

### Editando o batch
`config/batch_runs.yaml` define threads totais e a lista de agentes:
```yaml
threads: 4                  # 0 ou <=0 usa std::thread::hardware_concurrency()
agents:
  - name: greedy_baseline
    type: greedy            # random | greedy | mcts_rollout
    episodes: 20
  - name: random_baseline
    type: random
    episodes: 20
  - name: mcts_default
    type: mcts_rollout
    episodes: 20
    mcts_config: agents/mcts_rollout/config.yaml  # opcional; usa busca padrão se omitido
```
- Cada agente roda com até `threads` jogos simultâneos (limitado pelo número de episódios).
- MCTS lê parâmetros de YAML simples (seed, iterations, rollout_depth, uct_c); o default fica em `agents/mcts_rollout/config.yaml`.

### Saída e logs
- Cada agente grava `agents/<agent_dir>/run_<runId>.csv` (ex.: `agents/heuristic_greedy/run_YYYYMMDD_HHMMSSmmm_greedy.csv`).
- Colunas: `run_id,episode_index,agent_name,mode_name,score,total_lines,total_turns,holds_used,elapsed_seconds,agent_config`.
- `run_id` é um timestamp; `agent_config` inclui o snapshot da config do MCTS quando aplicável.

## GUI opcional (SFML)
- Requer SFML 2.5+ disponível no sistema.
- Configure com `cmake -S . -B build -DBUILD_TETRIS_GUI=ON -DCMAKE_BUILD_TYPE=Release`.
- Compile e rode `./build/tetris_gui` (Linux/macOS) ou `.\build\Release\tetris_gui.exe` (Windows) para jogar ou testar os modos de IA com interface gráfica.