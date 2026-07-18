# ESTADO ATUAL — AtivaStage

> Checkpoint vivo para retomada entre sessões. Como/quando atualizar:
> [`docs/METODO_ESTADO.md`](docs/METODO_ESTADO.md).

- **Última atualização:** 2026-07-18 (Fase 1 iniciada: Marco 1.0 no GitHub +
  esqueleto do Marco 1.1 — ver `docs/plan/PLANO_FASE_1.md`)
- **Fase corrente:** Fase 0 — Fundação **CONCLUÍDA ✅** (CI verde nas 2 plataformas,
  run `ec3d0c8`, 2m18s). Próxima: Fase 1 — Prova audiovisual.
- **Versão:** 0.0.1 (ver arquivo `VERSION`)
- **Branch sugerida:** `phase/0-foundation`

---

## O que já está feito

- [x] Estrutura do repositório conforme Seção 3 do plano (`src/`, `tests/`,
      `tools/`, `docs/`, `.github/`, `mobile/`, `scripts/`).
- [x] Build system: `CMakeLists.txt` raiz, `CMakePresets.json`
      (mac/win · debug/release), `vcpkg.json` (ffmpeg, sqlite3+fts5, catch2,
      libsodium).
- [x] `libpal`: interfaces `IPathService` e `IScreenService` (C++ puro, sem Qt),
      com implementações mac/win e factory `Pal.cpp` (seleção por plataforma —
      único lugar com `#ifdef`). Testes Catch2.
- [x] `libpersistence`: `Database` (wrapper SQLite, WAL, foreign_keys) +
      `Migrator` (tabela `meta`, `schema_version`, migrações numeradas em
      transação, migração 1 = baseline). Testes Catch2.
- [x] `AtivaStage.command`: auto-compilar adaptado p/ Qt+CMake (Homebrew local,
      compila libs + roda testes; monta `.app` com macdeployqt a partir da Fase 1).
- [x] CI GitHub Actions: build + testes em macOS e Windows.
- [x] ADR-0001 (stack) e ADR-0002 (auto-compilar adaptado).
- [x] Método de checkpoint de estado documentado.
- [x] Plano e método originais copiados para `docs/plan/`.
- [x] Verificação: todo o C++ de `libpal`/`libpersistence` compila com g++ -std=c++20
      (pegou e corrigiu um `#include <memory>` faltando nos ScreenService); o motor
      de migrações foi executado de verdade contra SQLite (memória → v1 →
      `app_bootstrap` → idempotente). CMake/YAML/JSON validados.

## Compilação real (macOS) — VERDE ✅

- `AtivaStage.command` rodou de ponta a ponta no Mac do Bruno (Qt 6 + SQLite
  3.51 via Homebrew): configura → compila 19 alvos → **2/2 testes passaram**
  (`pal_tests`, `persistence_tests`). Critério de saída principal da Fase 0
  atendido no ambiente de desenvolvimento.
- Correções aplicadas: (a) CMake raiz guardado por `EXISTS src/app/CMakeLists.txt`;
  (b) alvo SQLite trocado de `SQLite::SQLite3` (depreciado) para
  `SQLite3::SQLite3`.

## Git / GitHub

- Repositório: **https://github.com/bdsollz/ativastage** (branch `phase/0-foundation`).
- Commit inicial `c005644` enviado com sucesso (push exigiu PAT classic com
  escopos `repo` + `workflow`). Helpers de duplo-clique em `scripts/`:
  `git-setup.command`, `git-push.command`.

## O que falta na Fase 0 (próximos passos concretos)

1. ~~CI verde nas duas plataformas~~ **FEITO** (`ec3d0c8`). O CI foi reescrito:
   vcpkg pré-instalado do runner + `x-update-baseline`, `msvc-dev-cmd` no Windows,
   sem Qt nos presets debug.
2. **Pipeline de empacotamento mínimo** (dmg/zip sem assinatura) — esboço (opcional,
   pode entrar junto da Fase 9).

## Próxima fase — Fase 1 (Prova audiovisual) ⚠ fase de risco

**Plano de implementação detalhado: [`docs/plan/PLANO_FASE_1.md`](docs/plan/PLANO_FASE_1.md)**
(marcos 1.0–1.6, ordem, arquivos, critérios de aceite, matriz de HW, gate).

**Progresso (branch `phase/1-av-proof`):**
- **Marco 1.0 — no GitHub** (commit `83a8994`): ffmpeg+miniaudio de volta no
  `vcpkg.json`; alvo Qt Quick `app` (`src/app/`, janela vazia); `ATIVASTAGE_BUILD_APP=ON`
  nos presets debug; Qt 6.9.1 + cache binário do vcpkg no CI. ⚠ **Falta confirmar
  CI verde** (1º build com Qt+ffmpeg — pode precisar ajuste).
- **Marco 1.1 — esqueleto pronto (local, ainda não commitado):**
  - `libpal`: `IAudioDeviceService` + `AudioDeviceInfo` (factory cross-platform,
    stub determinístico; backend miniaudio real depois).
  - `libaudio` (novo módulo): núcleo puro e RT-safe — `DeckState`/`DeckStateMachine`,
    `RingBuffer` SPSC lock-free, `GainRamp`, `AudioDeck` com `renderInto` (corpo do
    callback). Testes `audio_tests` (Catch2). **Compilado e testado com g++ local:
    todos os checks passaram.**
  - ADR-0004 (miniaudio + contrato do callback RT).
  - **Falta na fiação do 1.1:** decoder FFmpeg (produtor do ring) + backend miniaudio
    real de `IAudioDeviceService` + integração no `app`.

Objetivo: provar o núcleo difícil antes de tudo. Entregas:
- Alvo GUI `app` (Qt Quick) — cria `src/app/CMakeLists.txt`; a partir daqui o
  `AtivaStage.command` passa a montar o `.app` sozinho.
- Janela de saída fullscreen em monitor secundário (mac + win).
- Vídeo H.264 1080p/4K via FFmpeg → textura RHI, com áudio sincronizado.
- Pré-carregamento (`Primed`) e GoLive < 100 ms.
- Audio Engine mínimo (1 deck: play/pause/stop/volume/fade, device selecionável).
- Hot-plug: desconectar monitor durante reprodução não derruba o app.
- Soaktest de 1 h nas duas plataformas.
- **Reintroduzir ffmpeg (+ libsodium quando o remote começar) no `vcpkg.json`** e
  no CI (agora vale o custo do build).

## Notas

- `vcpkg.json` enxugado p/ Fase 0: só `catch2` + `sqlite3[fts5]`.
  **ffmpeg e libsodium voltam na Fase 1** (quando forem de fato usados/compilados;
  evita build de ffmpeg from-source no CI agora). O proof local de ffmpeg já roda
  via Homebrew no `AtivaStage.command`.
- Helpers git em `scripts/`: `git-setup` (init+commit), `git-push` (remote+push),
  `git-sync` (add+commit+push, para iterar).

## Como compilar/testar agora

```bash
# Opção A — duplo-clique (usuário): AtivaStage.command
# Opção B — manual (dev, macOS), sem GUI ainda:
cmake -S . -B build/local-debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DATIVASTAGE_BUILD_TESTS=ON
cmake --build build/local-debug
ctest --test-dir build/local-debug --output-on-failure
```
(O preset `mac-debug`/`win-debug` usa a toolchain do vcpkg via `VCPKG_ROOT`.)

## Referência de UI (telas) — IMPORTANTE

O idealizador enviou o layout visual final (export do Google Stitch, "Studio
Precision"). **Tudo está no repositório:**

- `docs/design/UI_REFERENCE.md` — guia consolidado: tokens (cores de estado
  LIVE=vermelho, PREVIEW=âmbar, ACTIVE=azul, PLAYING=verde), tipografia (Inter +
  JetBrains Mono), shell global (sidebar Music/Bible/Presentation/Audio/Library/
  Settings + barra de transporte PREV/NEXT/CLEAR/BLACK/LOGO/EMERGENCY), e o
  layout de cada uma das 5 telas.
- `docs/design/stitch/` — arquivos brutos: 5 telas com `screen.png` (mockup) e
  `code.html` (markup Tailwind exato) + `studio_precision/DESIGN.md` (design system).

Telas de referência: Music Central, Bible Central, Audio Central (+ variante
"layout alinhado"), Saídas/Hardware (Output Routing).

**Quando usar:** a UI começa na **Fase 3** (Música). Fases 1–2 são núcleo/engines,
não precisam disso ainda. Ao construir QML, traduzir os tokens para um Theme
singleton (nunca hardcodar hex). A marca "ProChurch AV" nos mockups é placeholder
→ substituir por **AtivaStage** (nome definitivo, ADR-0003).

## Invariantes ativas (NÃO violar)

- Ordem das fases é lei; caminho crítico (engine) antes de UI.
- `Engines` nunca dependem de UI; `Core` nunca depende de `Engines`.
- Nenhum `#ifdef Q_OS_*` fora de `libpal`.
- Nada de RTF: texto sempre puro/estruturado.
- Nada de alocação/lock/I/O/SQLite no thread de áudio (quando existir).
- UI em pt-BR (`tr()`); código/commits/docs técnicos em inglês.
- Schema muda só por migração numerada + ADR.
- Merge na `main` só com CI verde nas duas plataformas.

## Decisões fechadas recentes

- **Nome do produto = AtivaStage** (definitivo, ADR-0003). Bundle id
  `br.com.ativa.ativastage`. Substituir placeholder "ProChurch AV" na UI.

## Decisões em aberto relevantes

- Nome definitivo do serviço mDNS (proposta `_ativastage._tcp.local`, confirmar
  na Fase 8).
- Baseline/pin do vcpkg (quando ffmpeg voltar, na Fase 1).
- Versão(ões) bíblica(s) em domínio público a incluir (Fase 4).

## Pendências / bloqueios

- Máquina/VM Windows com GPU para testes manuais (necessária a partir da Fase 3).
- Conta GitHub com Actions habilitado (runners macOS + Windows) para o CI rodar.

## Ponteiros

- Plano técnico: `docs/plan/Plano_Tecnico_Desenvolvimento.md` (fonte de "o quê/como").
- ADRs: `docs/decisions/`.
- Últimos arquivos criados: toda a fundação (ver "O que já está feito").
