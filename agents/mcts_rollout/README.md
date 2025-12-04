# MCTS Rollout Agent

Agente de Monte Carlo Tree Search guiado por rollouts com `GreedyAgent`. Usa UCT para seleção, recompensa interna baseada em `scoreDelta` e rollouts greedy com fallback aleatório defensivo.

## Como usar
- O batch runner reconhece `type: mcts_rollout`. Exemplo:

```yaml
threads: 8            # orçamento total de threads
agents:
  - name: mcts_default
    type: mcts_rollout
    episodes: 10
    mcts_config: agents/mcts_rollout/config.yaml
```

- Episódios MCTS são executados de forma sequencial; o orçamento de `threads` do runner é repassado para o próprio MCTS acelerar cada jogo.
- Se `mcts_config` não for informado, o runner procura por `agents/mcts_rollout/config.yaml` ou `config/mcts_rollout.yaml`.

## Configuração (YAML)
- `iterations` (**obrigatório**): simulações por decisão.
- `rollout_depth` / `maxDepth` (**obrigatório**): limite de profundidade das simulações.
- `uct_c` / `exploration` (**obrigatório**): constante de exploração.
- `threads` (opcional): threads usadas dentro do MCTS; se omitido, o runner define com base em `threads` global.
- `seed` (opcional): fixa o RNG do MCTS.

Exemplo em `agents/mcts_rollout/config.yaml`:

```yaml
seed: 1337
iterations: 5000
rollout_depth: 100
uct_c: 1.4142
# threads: 8  # opcional; se omitido, usa o valor fornecido pelo batch runner
```

## Funcionamento
- Seleção e expansão seguem UCT; rollouts usam `GreedyAgent` para trajetórias informativas.
- Cada thread recebe um RNG próprio (com seed derivada) e os resultados são agregados antes da decisão.
- O relatório de saída inclui a string de configuração (iterations, maxDepth, exploration, threads, seed) para reprodutibilidade.
