# MCTS Default Agent (rollouts aleatórios)

Alias para o agente MCTS unificado com `rollout_policy: random`, `reward_mode: score` e sem tabela de transposição. Seleção segue UCT; recompensas internas usam `scoreDelta`.

## Como usar
- No batch runner, use `type: mcts_default`.

```yaml
threads: 8
agents:
  - name: mcts_default
    type: mcts_default
    episodes: 10
    mcts_config: agents/mcts_default/config.yaml
```

- Episódios MCTS são executados de forma sequencial; o valor de `threads` do runner é repassado para o próprio MCTS acelerar cada jogo.
- Se `mcts_config` não for informado, o runner procura por `agents/mcts_default/config.yaml`, `config/mcts_default.yaml` ou os caminhos equivalentes em `..`.

## Configuração (YAML)
Mesmos campos do MCTS greedy:
- `iterations`, `rollout_depth`/`maxDepth`, `uct_c`/`exploration` (obrigatórios)
- `threads`, `seed`, `score_limit`, `time_limit_seconds` (opcionais)
- Campos extras aceitos pelo agente unificado: `rollout_policy`, `reward_mode`,
  `use_transposition_table`, `tt_max_entries`.

Exemplo em `agents/mcts_default/config.yaml`:

```yaml
seed: 1337
iterations: 200
rollout_depth: 10
uct_c: 1.4142
score_limit: 0
time_limit_seconds: 10
```
