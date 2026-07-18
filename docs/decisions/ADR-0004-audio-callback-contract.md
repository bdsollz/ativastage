# ADR-0004 — Backend de áudio (miniaudio) e contrato do callback de tempo real

- **Status:** Aceito
- **Data:** 2026-07-18
- **Contexto:** Início da Fase 1, Marco 1.1 (Audio Engine mínimo, 1 deck). O plano
  técnico (§1, §5.2) já fixou **miniaudio** como backend de saída e exige que o
  callback de áudio rode em thread de tempo real **sem alocação, sem locks, sem
  I/O e sem SQLite**. Este ADR registra como isso é imposto no código.

## Decisão

### Backend de dispositivo
- Saída de áudio via **miniaudio** (WASAPI no Windows, CoreAudio no macOS),
  encapsulado atrás de `pal::IAudioDeviceService` (enumeração, dispositivo padrão,
  seleção, notificação de hot-swap). miniaudio é multiplataforma, então essa
  interface tem **uma única implementação** (não há arquivos `mac/`+`win/`) e
  nenhum `#ifdef Q_OS_*` — coerente com a invariante da Seção 2.4.
- No Marco 1.1 a implementação é um **stub determinístico** (sem dependência),
  para permitir build e testes headless; o backend real de miniaudio a substitui
  quando a reprodução real entra, **sem mudar a interface**.

### Contrato do callback de tempo real
O caminho de render (`audio::AudioDeck::renderInto`) é o corpo do callback e
obedece:

1. **Sem alocação:** toda memória é alocada na construção (ring buffer, rampa).
2. **Sem locks:** comunicação control→audio por `std::atomic` e um **ring buffer
   SPSC lock-free** (`audio::RingBuffer`), único canal decode→audio.
3. **Sem I/O nem SQLite** dentro do callback.
4. **Sem inspeção de `DeckState`:** o thread de áudio lê apenas atomics
   (`playing_`, requisição de fade/volume) — a máquina de estados
   (`DeckStateMachine`) vive no thread de controle e não pode correr com o render.
5. **Underrun = silêncio**, nunca lixo (zero-fill quando o ring esvazia).
6. **Fades sem clique:** rampa de ganho por frame (`audio::GainRamp`), calculada
   no callback. Curva do MVP = **linear**; log/equal-power ficam para depois com a
   mesma interface. Fade-out = `setTarget(0, N)`; ao chegar a zero, o deck sinaliza
   conclusão para o controle finalizar `FADE OUT → PRONTO`.

### Estados do deck
Exatamente os do relatório (Seção 8), expostos ao `AppState` e ao mobile:
`PRONTO`, `CARREGANDO`, `TOCANDO`, `PAUSADO`, `FADE OUT`, `FALHA`
(`audio::DeckState`, rótulos pt-BR em `deckStateLabelPtBr`).

## Consequências
- O núcleo do `libaudio` (state machine, ring buffer, rampa, render) é
  **dependency-free e testável headless** — coberto por `audio_tests` e verificado
  fora do CI compilando com `g++ -std=c++20`.
- O decoder FFmpeg (produtor que alimenta o ring buffer) e o backend miniaudio de
  `IAudioDeviceService` são as próximas peças de fiação do Marco 1.1; ambos entram
  sem alterar o contrato aqui definido.
- Verificação futura sugerida: assert em debug que falha se `malloc` for chamado no
  thread de áudio (instrumentação), reforçando o item 1.

## Fecha
Registra a decisão "miniaudio + contrato do callback RT" prevista no
`docs/plan/PLANO_FASE_1.md` (§6). Não altera schema (sem migração).
