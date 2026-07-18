# AtivaStage

Programa de projeção e operação audiovisual para igrejas.
Stack: **Qt 6 (QML) + C++20 + CMake + vcpkg + FFmpeg + SQLite**.
Desenvolvimento em macOS; plataforma principal de produção: **Windows**.

> Fonte de verdade técnica: [`docs/plan/Plano_Tecnico_Desenvolvimento.md`](docs/plan/Plano_Tecnico_Desenvolvimento.md).
> Decisões que divergem do plano são registradas como ADR em [`docs/decisions/`](docs/decisions/).

## Estado atual

Consulte [`ESTADO_ATUAL.md`](ESTADO_ATUAL.md) — é o checkpoint vivo do projeto,
atualizado ao fim de cada sessão de desenvolvimento para permitir a retomada.

**Fase corrente:** Fase 0 — Fundação (em andamento).

## Instalar / atualizar no macOS (método de duplo-clique)

Este projeto é distribuído como **código-fonte + um script de auto-compilação**.
Não precisa de conta paga de desenvolvedor.

1. Coloque esta pasta em `~/AtivaStage` (evite `~/Downloads`).
2. Dê **dois cliques** em `AtivaStage.command`.
   - Na primeira vez ele instala o que faltar (Ferramentas Apple, Homebrew, Qt,
     CMake, Ninja, FFmpeg), compila e monta o `AtivaStage.app` em `/Applications`.
   - Rodar de novo quando chegar código novo = **atualizar**.

> A primeira compilação de um projeto Qt + FFmpeg pode demorar bastante e baixar
> vários GB. É esperado. Ver `docs/decisions/ADR-0002-auto-compilar-qt.md`.

## Compilar manualmente (desenvolvedores)

```bash
# macOS (Debug)
cmake --preset mac-debug
cmake --build --preset mac-debug
ctest --preset mac-debug
```

## Estrutura

```
src/            módulos (libcore, liblibrary, libpersistence, libpal, ... , app)
tests/          testes unitários / integração / assets
tools/          dbtool, soaktest
docs/           plan/ · decisions/ (ADRs) · protocol/ · db/
scripts/        utilitários de desenvolvimento
mobile/         app Flutter (Fase 8)
AtivaStage.command   instalador/atualizador de duplo-clique (macOS)
VERSION         fonte da verdade da versão (instalar vs atualizar)
```

## Convenções

- Código, commits e docs técnicos em **inglês**; UI em **pt-BR** (`tr()`).
- `Engines` nunca dependem de UI; `Core` nunca depende de `Engines`.
- Nenhum `#ifdef Q_OS_*` fora de `libpal`.
- Nada de RTF: texto sempre puro/estruturado.
- Merge na `main` só com CI verde nas duas plataformas.
