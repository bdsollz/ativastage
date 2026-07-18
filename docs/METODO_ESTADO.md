# Método de checkpoint de estado (retomada entre sessões)

O desenvolvimento do AtivaStage é assistido por IA em sessões que podem terminar
por limite de tokens. Para nunca perder o fio, mantemos um **checkpoint vivo** em
[`ESTADO_ATUAL.md`](../ESTADO_ATUAL.md) — um resumo que permite a uma sessão nova
retomar exatamente de onde a anterior parou, sem reler todo o histórico.

## Regra de ouro

> **Gerar/atualizar o `ESTADO_ATUAL.md` ANTES de a sessão acabar.**
> Na dúvida, atualizar cedo. Um checkpoint levemente adiantado é barato; perder
> o contexto de uma sessão inteira é caro.

## Quando atualizar o checkpoint

1. **Proativamente**, quando o orçamento de tokens da sessão estiver ficando
   baixo (sinal de que a sessão pode encerrar).
2. Ao **concluir uma entrega** com critério de saída atendido.
3. Ao **fechar um ADR** ou mudar uma decisão de arquitetura.
4. Ao **encerrar a sessão** por qualquer motivo — sempre deixar o checkpoint no
   estado real do repositório.

## O que o checkpoint deve conter (sempre)

- **Fase corrente** e progresso dentro dela (o que já foi feito, o que falta).
- **Próximos passos concretos** — a primeira coisa que a próxima sessão deve fazer.
- **Como compilar/testar agora** (comando exato).
- **Invariantes ativas** que não podem ser violadas.
- **Decisões em aberto** relevantes ao trabalho atual.
- **Pendências/bloqueios** conhecidos (ex.: precisa de máquina Windows real).
- **Ponteiros**: arquivos-chave tocados por último, ADRs recentes.

## Como uma sessão NOVA retoma

1. Ler `ESTADO_ATUAL.md` (fonte de verdade do progresso).
2. Ler o Plano Técnico em `docs/plan/` (fonte de verdade do "o quê"/"como").
3. Conferir o git log recente e o estado do CI.
4. Continuar a partir de "Próximos passos" do checkpoint.

## Relação com o auto-compilar

São dois "auto" distintos e complementares:

- **Auto-compilar** (`AtivaStage.command`, ADR-0002) = distribuição: duplo-clique
  instala/compila/atualiza o app na máquina do usuário.
- **Checkpoint de estado** (este método) = continuidade do desenvolvimento entre
  sessões de IA.

## Higiene de versão e commits

- `VERSION` é a fonte da verdade da versão do app (instalar-vs-atualizar).
- Commits pequenos e descritivos; branch por fase (`phase/0-foundation`, ...).
- Merge na `main` só com CI verde nas duas plataformas.
- Atualizar o `ESTADO_ATUAL.md` faz parte do "Definition of Done" da sessão.
