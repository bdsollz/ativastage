# ADR-0001 — Stack de tecnologia

- **Status:** Aceito
- **Data:** 2026-07-17
- **Contexto:** Fase 0 — Fundação

## Contexto

O AtivaStage precisa de multi-janela/multi-monitor maduro, renderização GPU,
pipeline audiovisual (decodificação de vídeo com sincronização A/V, áudio de
baixa latência), operação offline-first e portabilidade macOS (desenvolvimento)
→ Windows (produção principal).

## Decisão

Adotar a stack definida na Seção 1 do Plano Técnico:

| Área | Escolha |
|---|---|
| UI desktop | Qt 6 (Qt Quick/QML) |
| Linguagem | C++20 |
| Build | CMake ≥ 3.27 + Ninja (presets) |
| Dependências | vcpkg (manifest mode) para o CI reproduzível |
| Mídia | FFmpeg 7.x (config LGPL, só decodificação no MVP) |
| Renderização | Qt RHI / Qt Quick scene graph |
| Áudio | miniaudio (via camada `AudioDevice`) |
| Banco | SQLite + FTS5 |
| Licença Qt | LGPLv3, linkagem dinâmica |

## Consequências

- Linkagem dinâmica obrigatória com o Qt; o `.app`/instalador deve permitir
  substituição das bibliotecas Qt.
- FFmpeg compilado sem `--enable-gpl`.
- Nenhum `#ifdef Q_OS_*` fora de `libpal`.
- CI compila e testa nas duas plataformas desde a Fase 0.
