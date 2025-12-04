# Agents

Agrupa implementações e experimentos de agentes de IA que consomem o SDK. Cada subpasta define configurações reprodutíveis, documentação curta e dependências específicas.

## Runner em batch (sem GUI)

- Compile a partir da raiz com `cmake -B build -DCMAKE_BUILD_TYPE=Release` e `cmake --build build --config Release`.
- Rode `./build/tetris_batch_runner` (usa `config/batch_runs.yaml` por padrão) ou `./build/tetris_batch_runner config/minha_config.yaml`.
- Ajuste `config/batch_runs.yaml` para definir threads, agentes (`random`, `greedy` ou `mcts_rollout`), episódios e, opcionalmente, o caminho do YAML do MCTS.
- Resultados de cada agente vão para `agents/<agent_dir>/run_<runId>.csv` com as colunas `run_id,episode_index,agent_name,mode_name,score,total_lines,total_turns,holds_used,elapsed_seconds,agent_config`.
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
- `mcts_rollout`: episódios rodam de forma sequencial; o valor de `threads` do runner é repassado para o MCTS paralelizar cada jogo. Config YAML em `agents/mcts_rollout/config.yaml` ou `config/mcts_rollout.yaml`.
- `random`: não requer configuração.
