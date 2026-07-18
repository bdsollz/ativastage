# ADR-0002 — Distribuição por auto-compilação (adaptação Qt/CMake)

- **Status:** Aceito
- **Data:** 2026-07-17
- **Contexto:** Fase 0 — Fundação

## Contexto

`METODO_AUTO_COMPILAR.md` descreve distribuir **código-fonte + um script
`.command` de duplo-clique** que instala dependências, compila e monta o `.app`
localmente — sem conta paga de desenvolvedor, assinatura ou notarização. O
template original é escrito para Swift Package Manager (`swift build`), mas o
Plano Técnico (ADR-0001) exige Qt 6 + C++ + CMake.

## Decisão

Adaptar o método para a stack Qt/CMake, preservando os princípios (idempotência,
duplo-clique instala e atualiza, `VERSION` como fonte da verdade):

1. **Dependências locais via Homebrew** no `AtivaStage.command`
   (`qt`, `cmake`, `ninja`, `ffmpeg`, `sqlite`, `libsodium`, `catch2`). É o
   caminho mais simples para o usuário final de duplo-clique.
2. **vcpkg reservado ao CI** e a builds reproduzíveis (via presets +
   `VCPKG_ROOT`). O build local do `.command` **não** usa a toolchain vcpkg;
   configura o CMake apontando `CMAKE_PREFIX_PATH` para o Qt do Homebrew.
3. **Montagem do `.app`**: copia o binário, gera `Info.plist` a partir de
   `VERSION`, gera `.icns` de `icon.png` (se existir) e empacota as bibliotecas
   Qt com `macdeployqt` (consequência da linkagem dinâmica LGPL, ADR-0001).
4. **Compatível com fases:** enquanto não existir alvo GUI (Fase 0), o script
   compila as libs e roda os testes; a etapa de montar o `.app` ativa-se
   sozinha quando o executável `AtivaStage` passar a ser produzido (Fase 1).

## Consequências

- A primeira compilação (Qt + FFmpeg via Homebrew) pode demorar e baixar vários
  GB. É esperado e comunicado no README.
- Há duas rotas de dependências (Homebrew local, vcpkg no CI); divergências de
  versão são possíveis e devem ser observadas. Aceitável no MVP.
- A variante iOS do método (Xcode + XcodeGen) não se aplica: o app mobile é
  Flutter (Fase 8), fora deste script.
- Empacotamento assinado/notarizado (macOS) e instalador Windows continuam
  planejados para a Fase 9; este método cobre a distribuição de desenvolvimento
  e beta local.
