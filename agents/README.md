# Agents

Agrupa implementações e experimentos de agentes de IA que consomem o SDK. Cada subpasta define configurações reprodutíveis, documentação curta e dependências específicas.

As implementações em C++ de cada agente ficam dentro de `agents/<agente>/src` e são compiladas junto do target `tetris_env`; as execuções continuam gravando CSVs no próprio diretório do agente (`agents/<agente>/run_*.csv`).

## Runner em batch (sem GUI)

- Compile a partir da raiz com `cmake -B build -DCMAKE_BUILD_TYPE=Release` e `cmake --build build --config Release`.
- Rode `./build/tetris_batch_runner` (usa `config/batch_runs.yaml` por padrão) ou `./build/tetris_batch_runner config/minha_config.yaml`.
- Ajuste `config/batch_runs.yaml` para definir threads, agentes (`random`, `greedy`, `mcts_rollout` + variações antigas como alias), episódios e, opcionalmente, o caminho do YAML do MCTS.
- Resultados de cada agente vão para `agents/<agent_dir>/run_<runId>.csv` com as colunas `run_id,episode_index,agent_name,mode_name,score,total_lines,total_turns,holds_used,elapsed_seconds,end_reason,agent_config`.
- Use `--help` no executável para um exemplo rápido do formato do YAML e caminhos de saída.

Exemplo mínimo com o Greedy:

```yaml
threads: 4
agents:
  - name: greedy_baseline
    type: greedy
    episodes: 20
  - name: random_baseline
    type: random
    episodes: 20
```

Para o agente heurístico ganancioso, as execuções são gravadas em `agents/heuristic_greedy/` e a config de referência fica em `agents/heuristic_greedy/config.yaml`.

### Observações específicas
- `mcts_rollout`: agente MCTS unificado configurável (política de rollout greedy/random, recompensa score/heurística, tabela de transposição on/off). Episódios rodam de forma sequencial; o valor de `threads` do runner é repassado para o MCTS paralelizar cada jogo. Configs em `agents/mcts_rollout/*.yaml` (aliases antigos em `agents/mcts_greedy`, `agents/mcts_default`, `agents/mcts_transposition` continuam válidos).
- `random`: não requer configuração.
- `greedy`: não requer configuração.
