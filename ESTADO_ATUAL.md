# ESTADO ATUAL — AtivaStage

> Checkpoint vivo para retomada entre sessões. Como/quando atualizar:
> [`docs/METODO_ESTADO.md`](docs/METODO_ESTADO.md).

- **Última atualização:** 2026-07-17 23:16 (-03)
- **Fase corrente:** Fase 0 — Fundação (em andamento)
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

1. **CI verde** nas duas plataformas (macOS + Windows). 1ª execução (`c005644`)
   falhou em ~23s no setup do vcpkg (`run-vcpkg` com `master` inválido). CI
   reescrito: usa vcpkg pré-instalado do runner, `x-update-baseline` p/ baseline,
   `msvc-dev-cmd` no Windows, sem Qt (não usado nos presets debug da Fase 0).
   → Falta commitar+push (rodar `scripts/git-sync.command`) e conferir verde.
2. **Pipeline de empacotamento mínimo** (dmg/zip sem assinatura) — esboço.

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

## Invariantes ativas (NÃO violar)

- Ordem das fases é lei; caminho crítico (engine) antes de UI.
- `Engines` nunca dependem de UI; `Core` nunca depende de `Engines`.
- Nenhum `#ifdef Q_OS_*` fora de `libpal`.
- Nada de RTF: texto sempre puro/estruturado.
- Nada de alocação/lock/I/O/SQLite no thread de áudio (quando existir).
- UI em pt-BR (`tr()`); código/commits/docs técnicos em inglês.
- Schema muda só por migração numerada + ADR.
- Merge na `main` só com CI verde nas duas plataformas.

## Decisões em aberto relevantes

- Nome definitivo do produto e do serviço mDNS (bundle id provisório:
  `br.com.ativa.ativastage`).
- Baseline/pin do vcpkg (item 2 acima).
- Versão(ões) bíblica(s) em domínio público a incluir (Fase 4).

## Pendências / bloqueios

- Máquina/VM Windows com GPU para testes manuais (necessária a partir da Fase 3).
- Conta GitHub com Actions habilitado (runners macOS + Windows) para o CI rodar.

## Ponteiros

- Plano técnico: `docs/plan/Plano_Tecnico_Desenvolvimento.md` (fonte de "o quê/como").
- ADRs: `docs/decisions/`.
- Últimos arquivos criados: toda a fundação (ver "O que já está feito").
