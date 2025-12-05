# MCTS Rollout Agent (unificado)

Agente de Monte Carlo Tree Search configurável via YAML. Todas as variações de MCTS anteriores (rollout greedy/aleatório, recompensa por score ou heurística greedy, com/sem tabela de transposição) agora são parametrizadas neste mesmo agente.

## Como usar
- Use `type: mcts_rollout` no batch runner. Exemplo com quatro variações rodando na mesma execução:

```yaml
threads: 8            # orçamento total de threads
agents:
  - name: mcts_score_random_noTT
    type: mcts_rollout
    episodes: 20
    mcts_config: agents/mcts_rollout/score_random_noTT.yaml
  - name: mcts_score_random_tt
    type: mcts_rollout
    episodes: 20
    mcts_config: agents/mcts_rollout/score_random_tt.yaml
  - name: mcts_greedy_random_noTT
    type: mcts_rollout
    episodes: 20
    mcts_config: agents/mcts_rollout/greedy_random_noTT.yaml
  - name: mcts_greedy_random_tt
    type: mcts_rollout
    episodes: 20
    mcts_config: agents/mcts_rollout/greedy_random_tt.yaml
```

- Episódios MCTS são executados de forma sequencial; o orçamento de `threads` do runner é repassado para o próprio MCTS paralelizar cada jogo.
- Se `mcts_config` não for informado, o runner procura por `agents/mcts_rollout/config.yaml` ou `config/mcts_rollout.yaml`.

## Configuração (YAML)
- `iterations` (**obrigatório**): simulações por decisão.
- `rollout_depth` / `maxDepth` (**obrigatório**): limite de profundidade das simulações.
- `uct_c` / `exploration` (**obrigatório**): constante de exploração.
- `threads` (opcional): threads usadas dentro do MCTS; se omitido, o runner define com base em `threads` global.
- `seed` (opcional): fixa o RNG do MCTS.
- `score_limit` (opcional): encerra o episodio quando o score atingir esse valor.
- `time_limit_seconds` (opcional): encerra o episodio quando esse tempo for atingido.
- `rollout_policy`: `greedy` (default) ou `random`.
- `reward_mode`: `score` (default, usa scoreDelta) ou `greedy` (heurística do Greedy).
- `use_transposition_table`: ativa/desativa a TT.
- `tt_max_entries`: limite de entradas da TT (0 = usa default interno).

Exemplo em `agents/mcts_rollout/config.yaml` (rollout greedy, recompensa score):

```yaml
seed: 1337
iterations: 5000
rollout_depth: 100
uct_c: 1.4142
rollout_policy: greedy
reward_mode: score
use_transposition_table: false
tt_max_entries: 0
score_limit: 0
time_limit_seconds: 0
```

## Funcionamento
- Seleção e expansão seguem UCT; a política de rollout e a função de recompensa são escolhidas via YAML.
- Cada thread recebe um RNG próprio (com seed derivada) e os resultados são agregados antes da decisão.
- O relatório de saída inclui a string de configuração com os campos novos para reprodutibilidade.
