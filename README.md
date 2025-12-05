# TetrisAI

Ambiente de Tetris em C++20 voltado para experimentos de IA (Random, Greedy e variantes de MCTS com rollouts greedy/aleatórios e tabela de transposição). A biblioteca `tetris_env` expõe uma API simples para agentes e o projeto já inclui binários para rodar partidas de teste e execuções em batch sem GUI.

## Estrutura
- `include/` e `src/tetris_env/`: API e implementação do ambiente e infraestrutura compartilhada.
- `apps/`: executáveis `tetris_env_test` (smoke test) e `tetris_batch_runner` (execução em lote).
- `config/batch_runs.yaml`: configuração padrão para o runner em batch.
- `agents/`: implementações dos agentes em `agents/<agente>/src`, configs de referência e diretório onde os CSVs de resultados são gravados.
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
`config/batch_runs.yaml` define threads totais e a lista de agentes. Dicas rápidas:
- `threads`: orçamento global; 0 ou valor <=0 usa `std::thread::hardware_concurrency()`.
- Cada agente recebe um `name`, `type` (`random`, `greedy`, `mcts_rollout` ou aliases `mcts_*`) e `episodes`.
- Para `mcts_rollout`, a chave `mcts_config` aponta para um YAML específico do agente (pode ser relativo ao arquivo do batch).
- Campos permitidos em cada agente:
  - `name`: identificador livre (vira sufixo do CSV).
  - `type`: `random`, `greedy`, `mcts_rollout`, `mcts_greedy`, `mcts_default`, `mcts_transposition`.
  - `episodes`: inteiro > 0.
  - `mcts_config` (somente para tipos MCTS): caminho para o YAML do agente.

```yaml
threads: 4                  # 0 ou <=0 usa std::thread::hardware_concurrency()
agents:
  - name: greedy_baseline
    type: greedy            # random | greedy | mcts_rollout (alias mcts_*)
    episodes: 20
  - name: random_baseline
    type: random
    episodes: 20
  - name: mcts_score_random_noTT
    type: mcts_rollout
    episodes: 10
    mcts_config: agents/mcts_rollout/score_random_noTT.yaml
  - name: mcts_score_random_tt
    type: mcts_rollout
    episodes: 10
    mcts_config: agents/mcts_rollout/score_random_tt.yaml
  - name: mcts_greedy_random_noTT
    type: mcts_rollout
    episodes: 10
    mcts_config: agents/mcts_rollout/greedy_random_noTT.yaml
  - name: mcts_greedy_random_tt
    type: mcts_rollout
    episodes: 10
    mcts_config: agents/mcts_rollout/greedy_random_tt.yaml
```
- Cada agente roda com até `threads` jogos simultâneos (limitado pelo número de episódios).
- MCTS lê parâmetros de YAML simples (seed, iterations, rollout_depth, uct_c, limites opcionais de score/tempo) **e** novos campos `rollout_policy`, `reward_mode`, `use_transposition_table`, `tt_max_entries`. Exemplos ficam em `agents/mcts_rollout/*.yaml` (aliases antigos em `agents/mcts_greedy`, `agents/mcts_default`, `agents/mcts_transposition` continuam funcionando).
  - Para criar uma variante, copie um YAML existente em `agents/mcts_rollout/`, ajuste `rollout_policy`, `reward_mode` e `use_transposition_table`, e referencie-o no `mcts_config` do batch.
  - Se `mcts_config` não for informado, o runner procura `agents/mcts_rollout/config.yaml` e `config/mcts_rollout.yaml`.
- Campos permitidos em um `mcts_config`:
  - `iterations` (int > 0)
  - `rollout_depth` ou `maxDepth` (int > 0)
  - `uct_c` ou `exploration` (double > 0)
  - `threads` (opcional; int > 0, override interno)
  - `seed` (opcional; uint32)
  - `score_limit` / `max_score` / `scoreLimit` (opcional; int > 0)
  - `time_limit_seconds` / `time_limit` (opcional; double > 0)
  - `rollout_policy` (`greedy` | `random`, default greedy)
  - `reward_mode` (`score` | `greedy`, default score)
  - `use_transposition_table` (`true` | `false`, default false)
  - `tt_max_entries` (size_t; 0 usa limite interno)

### Saída e logs
- Cada agente grava `agents/<agent_dir>/run_<runId>.csv` (ex.: `agents/heuristic_greedy/run_YYYYMMDD_HH_MM_SS_greedy.csv`).
- Colunas: `run_id,episode_index,agent_name,mode_name,score,total_lines,total_turns,holds_used,elapsed_seconds,end_reason,agent_config`.
- `run_id` é um timestamp; `agent_config` inclui o snapshot da config do MCTS quando aplicável.

## GUI opcional (SFML)
- Requer SFML 2.5+ disponível no sistema.
- Configure com `cmake -S . -B build -DBUILD_TETRIS_GUI=ON -DCMAKE_BUILD_TYPE=Release`.
- Compile e rode `./build/tetris_gui` (Linux/macOS) ou `.\build\Release\tetris_gui.exe` (Windows) para jogar ou testar os modos de IA com interface gráfica.
