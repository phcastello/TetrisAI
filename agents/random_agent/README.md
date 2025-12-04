# Random Agent

Escolhe ações válidas de forma uniforme a cada passo. Serve como baseline simples para validar determinismo do ambiente e pipelines de benchmark.

## Como usar
- No batch runner, defina `type: random`:

```yaml
agents:
  - name: random_baseline
    type: random
    episodes: 20
```

- Não possui arquivo de configuração; qualquer campo `mcts_config` é ignorado.

## Funcionamento
- Em cada tick do ambiente, sorteia uma ação dentro de `getValidActions()`.
- Nenhuma memória interna ou parametrização adicional.
