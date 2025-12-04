# MCTS Greedy Agent

Agente de Monte Carlo Tree Search com rollouts guiados pelo `GreedyAgent`. Usa UCT para seleção, recompensa interna baseada em `scoreDelta` e fallback aleatório defensivo quando necessário.

## Como usar
- O batch runner reconhece `type: mcts_greedy` (ou o alias `mcts_rollout`). Exemplo:

```yaml
threads: 8
agents:
  - name: mcts_greedy
    type: mcts_greedy
    episodes: 10
    mcts_config: agents/mcts_greedy/config.yaml
```

- Episódios MCTS são executados de forma sequencial; o valor de `threads` do runner é repassado para o próprio MCTS acelerar cada jogo.
- Se `mcts_config` não for informado, o runner procura por `agents/mcts_greedy/config.yaml`, `config/mcts_greedy.yaml` ou os caminhos equivalentes em `..`.

## Configuração (YAML)
- `iterations` (**obrigatório**): simulações por decisão.
- `rollout_depth` / `maxDepth` (**obrigatório**): limite de profundidade das simulações.
- `uct_c` / `exploration` (**obrigatório**): constante de exploração.
- `threads` (opcional): threads usadas dentro do MCTS; se omitido, o runner define com base em `threads` global.
- `seed` (opcional): fixa o RNG do MCTS.
- `score_limit` (opcional): encerra o episodio quando o score atingir esse valor.
- `time_limit_seconds` (opcional): encerra o episodio quando esse tempo for atingido.

Exemplo em `agents/mcts_greedy/config.yaml`:

```yaml
seed: 1337
iterations: 5000
rollout_depth: 100
uct_c: 1.4142
score_limit: 0          # defina >0 para cortar episodios longos por score
time_limit_seconds: 0   # defina >0 para limitar a duracao em segundos
# threads: 8  # opcional; se omitido, usa o valor fornecido pelo batch runner
```

## Funcionamento
- Seleção e expansão seguem UCT; rollouts usam `GreedyAgent` para trajetórias informativas.
- Cada thread recebe um RNG próprio (com seed derivada) e os resultados são agregados antes da decisão.
- O relatório de saída inclui a string de configuração (iterations, maxDepth, exploration, threads, seed) para reprodutibilidade.
