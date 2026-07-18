# PLANO TÉCNICO DE DESENVOLVIMENTO
## Programa de Projeção e Operação Audiovisual para Igrejas

**Versão 1.0 — Julho de 2026**
**Documento-base:** Relatório de Visão do Produto v1.0
**Stack aprovada:** Qt 6 + QML + C++
**Ambiente de desenvolvimento:** macOS | **Plataforma principal de produção:** Windows
**Destinatário deste plano:** agente de desenvolvimento (Claude Opus)

---

## 0. Como usar este documento

Este plano é a fonte de verdade técnica do projeto. Ele traduz o Relatório de Visão em decisões de engenharia, estrutura de código, modelo de dados, fases de desenvolvimento e critérios de aceite. O agente de desenvolvimento deve:

1. Seguir as fases na ordem definida (Seção 10). Não iniciar uma fase sem os critérios de saída da anterior atendidos.
2. Nunca violar as invariantes arquiteturais da Seção 3.3.
3. Registrar decisões que divirjam deste plano em `docs/decisions/` (formato ADR) antes de implementá-las.
4. Manter o build Windows funcional desde a Fase 1 — Windows é a plataforma principal, mesmo com desenvolvimento no macOS.

---

## 1. Resumo das decisões técnicas

| Decisão | Escolha | Justificativa |
|---|---|---|
| Framework desktop | **Qt 6 (Qt Quick/QML para UI, Widgets não usado)** | Multi-janela/multi-monitor maduro, renderização GPU via RHI (Metal no macOS, D3D11/12 no Windows), ecossistema audiovisual comprovado |
| Versão do Qt | **Última release estável open-source (6.9+; verificar no início da Fase 0)** | LTS 6.8 tem patches restritos à licença comercial; open-source deve acompanhar a release estável corrente |
| Licença Qt | **LGPLv3, linkagem dinâmica** | Gratuito; exige DLLs/dylibs do Qt distribuídas separadamente (não linkar estaticamente) |
| Linguagem do núcleo | **C++20** | Padrão maduro no Qt 6, `std::optional`, ranges, coroutines onde útil |
| Build system | **CMake ≥ 3.27 + Ninja** | Padrão Qt 6, multiplataforma, suporte a presets |
| Gerência de dependências | **vcpkg (manifest mode)** | FFmpeg, libsodium etc. com builds reproduzíveis em macOS e Windows |
| Mídia (decodificação) | **FFmpeg 7.x (libav*)** | Codecs, demux, filtros, escala, resampling; mesmo princípio do EasyWorship |
| Renderização | **Qt RHI / Qt Quick scene graph** (Metal no macOS, D3D no Windows) | Compositor GPU sem escrever backend gráfico próprio; Skia descartado para reduzir dependências |
| Áudio (saída) | **miniaudio** (biblioteca única, header-only) via camada própria `AudioDevice` | Baixa latência, WASAPI no Windows / CoreAudio no macOS, seleção de dispositivo, sem custo de licença |
| Banco de dados | **SQLite + FTS5** (bundled, mesma versão nas duas plataformas) | Offline-first, pesquisa full-text, arquivo único |
| Descoberta de rede | **mDNS/Bonjour** — Apple Bonjour no macOS, implementação própria/qmdnsengine no Windows | Requisito do relatório |
| Protocolo mobile | **HTTP (REST, consultas) + WebSocket (estado e comandos), JSON, versionado** | Requisito do relatório; simples de testar |
| App mobile | **Flutter (iOS + Android)** — inicia na última fase do MVP | Decisão do relatório |
| Testes | **Qt Test + Catch2** para C++, testes QML com `qmltestrunner`, testes de integração via CLI headless | Cobertura do núcleo sem UI |
| CI | **GitHub Actions**: runners macOS + Windows a cada push na main | Builds e testes contínuos nas duas plataformas |
| Empacotamento | macOS: `.dmg` assinado/notarizado · Windows: instalador **Inno Setup** ou MSIX, assinado | Distribuição padrão de cada plataforma |
| Serialização de programação | Pacote `.service` = ZIP contendo SQLite interno + mídias referenciadas (princípio do `.ewsx`) | Programação autocontida |

### 1.1 Pontos de atenção da licença LGPL (Qt)

- Linkagem **dinâmica** obrigatória com as bibliotecas Qt (frameworks/dylibs no macOS, DLLs no Windows).
- O instalador deve permitir substituição das bibliotecas Qt (consequência natural da linkagem dinâmica).
- Não modificar o código-fonte do Qt; se for necessário, publicar as modificações.
- FFmpeg: compilar em configuração **LGPL** (sem `--enable-gpl`, sem x264/x265 encoders). Para o MVP só decodificamos, então LGPL é suficiente.
- NDI SDK (versão profissional): licença própria da NewTek/Vizrt — tratar apenas na fase Pro.

---

## 2. Visão da arquitetura

### 2.1 Camadas

```
┌────────────────────────────────────────────────────────┐
│  UI do Operador (QML)          Janelas de Saída (QML/RHI)│
├────────────────────────────────────────────────────────┤
│  ViewModels / Controllers (C++ QObject, expostos ao QML) │
├────────────────────────────────────────────────────────┤
│  CORE (C++ puro + Qt Core, SEM dependência de QML)       │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────────┐  │
│  │ AppState │ │ Commands │ │ Services │ │ EventStore │  │
│  └──────────┘ └──────────┘ └──────────┘ └────────────┘  │
├────────────────────────────────────────────────────────┤
│  ENGINES                                                 │
│  Presentation · Media (FFmpeg) · Audio · Output · Remote │
├────────────────────────────────────────────────────────┤
│  PLATFORM ABSTRACTION LAYER (PAL)                        │
│  Monitores · Dispositivos de áudio · Caminhos · mDNS ·   │
│  Energia/suspensão · Fontes                              │
├────────────────────────────────────────────────────────┤
│  Persistence (SQLite + FTS5 + migrações + backups)       │
└────────────────────────────────────────────────────────┘
```

### 2.2 Módulos (mapeados do relatório, Seção 11)

| Módulo | Biblioteca CMake | Responsabilidade |
|---|---|---|
| Core | `libcore` | Estado central (AppState), barramento de comandos, regras de operação, programação do culto |
| Library Service | `liblibrary` | CRUD e pesquisa de músicas, Bíblias, apresentações, mídias e áudios |
| Presentation Engine | `libpresentation` | Geração de slides, layout de texto, temas, transições, quebra automática |
| Media Engine | `libmedia` | Decodificação FFmpeg, pré-carregamento, clock de sincronização A/V, upload de frames para GPU |
| Audio Engine | `libaudio` | Decks, playlist, fades, crossfade, volumes, roteamento, pré-escuta |
| Output Manager | `liboutput` | Janelas de saída por monitor, layouts por destino (público/palco/transmissão), NDI (Pro) |
| Remote Server | `libremote` | HTTP + WebSocket, descoberta mDNS, pareamento, sessões, permissões |
| Event Store | `libevents` | Log append-only de execução (o que foi exibido/tocado, quando, por quem) |
| Persistence | `libpersistence` | SQLite, migrações versionadas, FTS5, snapshots, backups rotativos |
| PAL | `libpal` | Abstrações de plataforma (ver 2.4) |
| App | `app` | Executável: composição dos módulos, UI QML, janelas |

Cada módulo é uma biblioteca estática CMake com testes próprios. Dependências fluem de cima para baixo; **Engines nunca dependem de UI; Core nunca depende de Engines** (Core define interfaces, Engines implementam).

### 2.3 Estado único de operação (princípio central do relatório)

Todo o sistema observa um único objeto de estado (`AppState`), mutado exclusivamente por **comandos**:

```
UI desktop ──┐
Mobile ──────┼──► CommandBus ──► AppState (single-writer, thread principal)
Atalhos ─────┘                      │
                                    ├──► sinais Qt → UI desktop
                                    ├──► serialização → WebSocket (mobile)
                                    ├──► OutputManager (re-render das saídas)
                                    └──► EventStore (log de execução)
```

- `AppState` contém: programação aberta, item selecionado, conteúdo do **Preview**, conteúdo do **Live**, estado dos decks de áudio, estado das saídas, dispositivos conectados.
- Comandos são objetos serializáveis (`GoLive`, `NextSlide`, `PlayTrack`, `SetVolume`, `ClearOutput`, `ShowBlack`, `ShowLogo`…). O mesmo comando serve para teclado, mouse e mobile — o mobile envia literalmente o mesmo JSON do comando.
- Preview e Live são dois estados distintos; **nenhum comando de edição/seleção toca o Live**. Só `GoLive`/`ShowItem` promovem Preview → Live (regra operacional da Seção 9 do relatório).

### 2.4 Platform Abstraction Layer (PAL)

Interfaces C++ puras com implementações por plataforma (requisito da Seção 14 do relatório):

- `IScreenService` — enumeração de monitores, hot-plug (conexão/desconexão ao vivo), identificação persistente de monitor ("telão é sempre o HDMI-2").
- `IAudioDeviceService` — enumeração de dispositivos, dispositivo padrão, hot-swap, notificação de remoção.
- `IPathService` — diretórios de dados, mídia, backups, logs (`~/Library/Application Support/...` vs `%APPDATA%`).
- `IDiscoveryService` — publicação mDNS (`_igrejaproj._tcp.local`).
- `IPowerService` — inibir suspensão durante o culto; reagir a sleep/wake.
- `IFontService` — enumeração de fontes com fallback consistente entre plataformas.

**Regra:** nenhum `#ifdef Q_OS_MAC`/`Q_OS_WIN` fora de `libpal`. Isso é auditável e será verificado em revisão.

---

## 3. Estrutura do repositório

```
/
├── CMakeLists.txt
├── CMakePresets.json          # presets: mac-debug, mac-release, win-debug, win-release
├── vcpkg.json                 # ffmpeg, catch2, libsodium, miniaudio...
├── .github/workflows/ci.yml   # build+testes macOS e Windows
├── docs/
│   ├── decisions/             # ADRs (registros de decisão)
│   ├── protocol/              # especificação do protocolo mobile (versionada)
│   └── db/                    # schema e migrações documentadas
├── src/
│   ├── core/                  # libcore
│   ├── library/               # liblibrary
│   ├── presentation/          # libpresentation
│   ├── media/                 # libmedia
│   ├── audio/                 # libaudio
│   ├── output/                # liboutput
│   ├── remote/                # libremote
│   ├── events/                # libevents
│   ├── persistence/           # libpersistence
│   ├── pal/                   # libpal (+ pal/mac, pal/win)
│   └── app/                   # executável + QML
│       └── qml/               # telas do operador (layout provisório até o Stitches)
├── tests/
│   ├── unit/                  # por módulo
│   ├── integration/           # cenários ponta-a-ponta headless
│   └── assets/                # mídias de teste (vídeos curtos, áudios, imagens)
├── tools/
│   ├── dbtool/                # CLI: migrar, inspecionar, popular banco de exemplo
│   └── soaktest/              # teste de resistência (4h de reprodução contínua)
└── mobile/                    # app Flutter (criado na Fase 8)
```

---

## 4. Modelo de dados (SQLite)

Um único arquivo `library.db`, migrado por versões (`schema_version` na tabela `meta`). Tabelas conforme a Seção 12 do relatório:

### 4.1 Música
```sql
songs(id TEXT PK,            -- UUIDv7, imutável
      title, artist, author, copyright, ccli, key, tempo,
      tags, notes, default_arrangement,
      created_at, updated_at, revision INTEGER)
song_sections(id PK, song_id FK, kind,      -- verse|chorus|bridge|prechorus|ending|custom
              label, position, body TEXT)    -- texto puro + marcação leve, NUNCA RTF
song_revisions(id PK, song_id FK, revision, snapshot_json, created_at)
songs_fts(FTS5: title, artist, author, body) -- sincronizada por triggers
```

### 4.2 Bíblia
```sql
bible_versions(id PK, code, name, language, license_kind, installed_at)
bible_books(id PK, version_id FK, book_number, name, abbrev)
bible_verses(id PK, version_id FK, book_number, chapter, verse, text)
bible_fts(FTS5: text)                        -- pesquisa textual por versão
```
Versões bíblicas distribuídas como módulos `.bible` (SQLite avulso importado), respeitando licenciamento individual (Seção 6 do relatório). MVP inclui ao menos uma versão em domínio público em português.

### 4.3 Apresentação
```sql
presentations(id PK, title, aspect_ratio, base_width, base_height, theme_id, ...)
slides(id PK, presentation_id FK, position, master_slide_id, background_json)
elements(id PK, slide_id FK, kind,           -- text|image|video|shape|color|gradient|clock|countdown|dynamic
         z_order, geometry_json, style_json, content_ref)
master_slides(id PK, name, elements_json)
styles(id PK, name, kind, payload_json)      -- temas de música, de Bíblia e de apresentação
resources(id PK, media_asset_id FK, usage)
```

### 4.4 Mídia e áudio
```sql
media_assets(id PK, kind,                    -- image|video|audio
             sha256, duration_ms, width, height, codec_info_json, thumb_path)
media_locations(id PK, asset_id FK, path, volume_label, last_seen_at)  -- múltiplos caminhos p/ mesmo hash
audio_tracks(id PK, asset_id FK, title, artist, gain_db, cue_in_ms, cue_out_ms,
             fade_in_ms, fade_out_ms, category)  -- category: vinheta|fundo|efeito|geral
audio_playlists(id PK, name, kind)           -- persistente | sessão
audio_playlist_items(playlist_id FK, track_id FK, position)
```

### 4.5 Programação e execução
```sql
services(id PK, title, service_date, status, -- draft|ready|live|archived
         notes, created_at, updated_at)
service_items(id PK, service_id FK, position, group_label,
              kind,                          -- song|bible|presentation|audio|note
              source_id, overrides_json)     -- comportamento por item sem alterar biblioteca
service_item_snapshots(id PK, service_item_id FK, snapshot_json, media_manifest_json,
                       created_at)           -- cópia estável no salvamento
playback_events(id PK, ts, service_id, item_id, event_kind,  -- shown|played|paused|stopped|cleared
                duration_ms, operator, device_id)             -- APPEND-ONLY
remote_devices(id PK, name, platform, token_hash, role,      -- observer|lyrics|audio|full
               paired_at, revoked_at)
remote_sessions(id PK, device_id FK, connected_at, disconnected_at, ip)
sync_queue(id PK, entity, entity_id, op, payload_json, created_at)  -- reservado p/ V2
```

### 4.6 Regras de persistência (Seção 12 do relatório)

- UUIDv7 imutável para todo conteúdo; `revision` incrementado a cada alteração.
- SHA-256 de toda mídia importada; duplicatas detectadas por hash; arquivo movido/renomeado é re-localizado por hash.
- Snapshot dos itens ao salvar programação: `service_item_snapshots` guarda o conteúdo congelado + manifesto de mídias.
- Migrações numeradas, com transação, backup automático do `.db` antes de migrar (Seção 15).
- Backups rotativos: a cada abertura do app, cópia do banco se o último backup tiver > 24 h; reter 14.
- WAL mode ativado; checkpoints em momentos ociosos, nunca durante Live.

---

## 5. Núcleo audiovisual (o coração do risco técnico)

### 5.1 Media Engine (vídeo)

- Thread de demux/decodificação por mídia ativa (FFmpeg): `avformat` → filas de pacotes → `avcodec` (com aceleração de hardware: VideoToolbox no macOS, D3D11VA no Windows, fallback software).
- Frames de vídeo entregues como texturas ao scene graph do Qt Quick via `QSGVideoNode`/textura RHI externa — zero cópia quando o decoder de hardware permitir.
- **Clock mestre = áudio.** Vídeo sincroniza descartando/segurando frames. Vídeo sem áudio usa clock monotônico.
- **Pré-carregamento:** quando um item de vídeo entra no Preview, o engine abre o arquivo, decodifica os primeiros ~500 ms e pausa pronto (estado `Primed`). `GoLive` inicia em < 100 ms sem tela preta (requisito NFR).
- Loop de vídeo sem "pisca" (seek para 0 com frame já decodificado).
- Formatos-alvo do MVP: H.264/HEVC em MP4/MOV, VP9/AV1 em WebM (decode), MP3/AAC/WAV/FLAC (áudio), JPEG/PNG/WebP (imagem).

### 5.2 Audio Engine (Central de Áudio — Seção 8 do relatório)

Arquitetura de decks conforme o relatório:

```
Playlist/fila → Deck A (principal) ─┐
                Deck B (principal) ─┼─ mixer ─ limiter ─→ Saída principal
                                    │
                Deck de pré-escuta ─┴───────────────────→ Saída de fone (V2)
```

- Dois decks principais permitem **crossfade** real entre faixas.
- Callback de áudio (miniaudio) em thread de tempo real: **sem alocação, sem locks, sem SQLite** dentro do callback. Comunicação por ring buffers e atomics.
- Fades (in/out/cross) calculados por rampa no callback, com curvas configuráveis (linear/log).
- Estados do player exatamente como a tabela do relatório: `PRONTO`, `CARREGANDO`, `TOCANDO`, `PAUSADO`, `FADE OUT`, `FALHA` — expostos no `AppState` e portanto visíveis no desktop e no mobile.
- Proteção contra reprodução simultânea acidental: iniciar uma faixa quando outra toca exige política explícita (substituir com fade | sobrepor autorizado | bloquear).
- Volume geral + volume por faixa (`gain_db`), pontos de entrada/saída (`cue_in/cue_out`).
- Todo evento (play, pause, stop, fim natural) gera registro no EventStore com data, hora, duração e operador.

### 5.3 Presentation Engine

- Pipeline: conteúdo semântico (seções da música / versículos / elementos) + tema → **slides resolvidos** (lista de elementos posicionados) → scene graph QML nas janelas de saída.
- Quebra automática de texto em slides: medição de texto real (QTextLayout) contra a caixa do tema; algoritmo idêntico para música e Bíblia (Seção 6: mesmo motor).
- Temas reutilizáveis (fonte, margens, alinhamento, fundo, transição) com herança: tema global → tema da central → override por item.
- Transições do MVP: corte e crossfade (200–400 ms, GPU). Demais na V2.
- Cada **saída** renderiza o mesmo estado Live com layout próprio (Seção 13): público (slide cheio), palco (letra atual + próxima + relógio, V2), transmissão (alpha, V2).

### 5.4 Output Manager

- Uma `QQuickWindow` sem moldura por saída física, posicionada no monitor designado, fullscreen.
- Mapeamento persistente monitor↔papel (público/palco/operador) com re-atribuição automática quando um monitor some/volta (hot-plug via PAL) — comportamento exigido pela Seção 14.
- Sem monitor secundário: saída pública vira janela flutuante redimensionável (modo ensaio).
- Comandos globais: `ShowBlack`, `ShowLogo`, `ClearText` (mantém fundo), `ClearAll`.

---

## 6. Protocolo mobile e Remote Server

### 6.1 Transporte

- HTTP/1.1 local (porta dinâmica anunciada via mDNS) para consultas (bibliotecas, programação, thumbnails).
- WebSocket para: (a) push de estado — o `AppState` serializado em snapshots + deltas; (b) envio de comandos — o mesmo JSON do `CommandBus`.
- **Versionamento desde o dia 1:** todo payload carrega `"v": 1`; handshake negocia a versão; especificação mantida em `docs/protocol/v1.md` (requisito da Seção 10).

### 6.2 Pareamento e segurança (Seções 10 e 15)

1. Mobile descobre o desktop via mDNS.
2. Solicita pareamento → desktop exibe código de 6 dígitos (e QR).
3. Handshake estilo PAKE (SPAKE2 via libsodium) → token persistente por dispositivo (`remote_devices.token_hash`).
4. Toda sessão subsequente autentica por token; TLS local com certificado autoassinado gerado na primeira execução (pinning pelo app após pareamento).
5. Papéis: `observer` | `lyrics` | `audio` | `full` — o CommandBus valida permissão por comando.
6. Revogação de dispositivo na UI do desktop; reconexão automática sem interromper a apresentação.

### 6.3 App Flutter (Fase 8)

- Telas do MVP mobile: programação + item atual, controle de slides (preview/live/avançar/voltar/limpar/preto/logo), player de áudio com indicador TOCANDO grande, pesquisa nas bibliotecas.
- Estado espelhado por WebSocket; fila de comandos com confirmação (ack + estado resultante).
- Nenhuma lógica de negócio no app: ele é uma vista remota do `AppState`.

---

## 7. Segurança, estabilidade e recuperação (Seção 15)

- **Autosave:** programação salva a cada mudança (debounce 2 s) em transação; estado da sessão (item atual, posição do áudio, saídas) persistido a cada 5 s em `session_state.json`.
- **Recuperação:** após encerramento inesperado, o app oferece restaurar a última sessão (programação + item ativo).
- **Pré-validação do culto:** botão "Verificar culto" varre todos os itens da programação, confere existência/hash das mídias, codecs suportados, fontes instaladas; relatório de problemas antes do culto.
- **Watchdog de mídia:** decodificadores rodam em threads supervisionadas; travou > 2 s → mata e reinicia o pipeline daquela mídia, mostrando fundo do tema no lugar (nunca tela preta congelada).
- **Logs:** spdlog ou categoria Qt, com rotação (10 arquivos × 5 MB), exportação em ZIP para suporte, sem dados sensíveis.
- **Modo seguro:** flag `--safe-mode` abre sem restaurar sessão, sem dispositivos de áudio custom e com saída única.
- **Zero nuvem no caminho crítico:** nenhuma chamada de rede externa é necessária para operar (offline-first).

---

## 8. Estratégia macOS → Windows (Seção 14)

Windows é a plataforma principal de produção. Regras vinculantes:

1. **CI desde a Fase 0:** GitHub Actions compila e roda testes em macOS e Windows a cada push. Um push que quebra o build Windows é tratado como build quebrado, ponto.
2. **Fase 1 (prova audiovisual) executa nas duas plataformas** antes de qualquer código de biblioteca/UI ser escrito.
3. Máquina Windows real (ou VM com GPU) para testes manuais semanais a partir da Fase 3: multi-monitor real, hot-plug de HDMI, troca de dispositivo de áudio, suspensão/retorno.
4. Diferenças conhecidas a testar explicitamente: caminhos e nomes de arquivo (encoding, `\` vs `/`, unidades removíveis), DPI/escala por monitor (Windows mistura escalas), decoders de hardware distintos, comportamento de fullscreen multi-monitor, fontes disponíveis.
5. Assinatura: macOS (Developer ID + notarização), Windows (certificado de code signing) — configurada na Fase 9, mas o pipeline de empacotamento existe desde a Fase 0.
6. Testes automatizados de renderização por **imagem de referência** (screenshot da saída → comparação com tolerância) rodando nas duas plataformas para detectar divergências visuais.

---

## 9. Qualidade e testes

| Nível | Ferramenta | O que cobre |
|---|---|---|
| Unitário | Catch2 | Regras do Core, quebra de slides, parsing, migrações, comandos |
| Integração | executável headless de teste | Fluxo completo: criar programação → preview → live → eventos gravados |
| Áudio/vídeo | soaktest CLI | 4 h de reprodução contínua com troca de faixas/vídeos; falha se houver glitch, leak (> X MB de crescimento) ou crash |
| Visual | comparação de imagem | Saídas renderizadas idênticas entre plataformas (tolerância definida) |
| Protocolo | testes de contrato | Cliente de teste valida cada mensagem contra `docs/protocol/v1.md` |
| Performance | benchs dedicados | Inicialização < 3 s com sessão restaurada; troca de slide < 50 ms; GoLive de vídeo primed < 100 ms; comando mobile → efeito < 200 ms; busca em 50 mil músicas < 150 ms |
| Acessibilidade | revisão manual (Fase 9) | Todas as ações críticas por teclado; contraste e tamanhos adequados a operação em sala escura (tema escuro padrão da UI do operador) |

Metas numéricas derivadas dos requisitos não funcionais da Seção 17 do relatório. O soaktest roda no CI noturno.

---

## 10. Fases de desenvolvimento — MVP

Cada fase tem objetivo, entregas e **critérios de saída** verificáveis. Estimativas assumem desenvolvimento contínuo assistido por IA com revisão humana.

### Fase 0 — Fundação (1–2 semanas)
**Objetivo:** esqueleto compilando e testando nas duas plataformas.
- Repositório com estrutura da Seção 3, CMake presets, vcpkg com FFmpeg compilando no macOS e Windows.
- CI verde nas duas plataformas (build + teste dummy).
- `libpal` com implementações mac/win de `IPathService` e `IScreenService` (enumeração básica).
- `libpersistence`: abertura de banco, tabela `meta`, motor de migrações + testes.
- Pipeline de empacotamento mínimo (dmg/zip sem assinatura).
- ADR-0001 registrando a stack.

**Saída:** `git clone` → build → testes passam em macOS e Windows via CI.

### Fase 1 — Prova audiovisual (2–3 semanas) ⚠ fase de risco
**Objetivo:** provar o núcleo difícil antes de tudo (recomendação da Seção 19 do relatório).
- Janela de saída fullscreen em monitor secundário (as duas plataformas).
- Vídeo H.264 1080p e 4K tocando fluido via FFmpeg → textura RHI, com áudio sincronizado.
- Pré-carregamento (`Primed`) e início < 100 ms.
- Audio Engine mínimo: um deck, play/pause/stop/volume/fade out, dispositivo selecionável.
- Hot-plug: desconectar o monitor durante reprodução não derruba o app.
- Soaktest de 1 h passando nas duas plataformas.

**Saída:** demo gravada nas duas plataformas; métricas de performance documentadas. **Gate de decisão:** problemas estruturais aqui param o projeto para reavaliação — nada acima foi construído ainda.

### Fase 2 — Core e estado (2 semanas)
**Objetivo:** espinha dorsal de comandos e estado.
- `AppState` completo (programação, seleção, Preview, Live, áudio, saídas).
- `CommandBus` com todos os comandos do MVP definidos e serializáveis.
- Separação Preview/Live imposta por tipo (impossível mutar Live sem `GoLive`).
- `EventStore` append-only gravando em SQLite.
- Autosave de sessão + restauração.
- Cobertura de testes do Core > 80 %.

**Saída:** testes de integração headless: montar programação fictícia, navegar, ir ao vivo, verificar eventos gravados.

### Fase 3 — Central de Música (2–3 semanas)
- Schema 4.1 + FTS5 + importação/exportação (texto puro estruturado; avaliar OpenLyrics).
- CRUD completo, seções semânticas, ordem de execução padrão e por culto.
- Presentation Engine v1: temas simples (fonte, cor, fundo de cor/imagem, margens), quebra automática em slides, transição de corte e crossfade.
- UI provisória do operador (QML funcional, layout simples — o visual final virá do Stitches): biblioteca, pesquisa, editor de letra, Preview/Live, saída pública.
- Histórico de execução por música (última vez, frequência).

**Saída:** operar um louvor completo de ponta a ponta com duas telas.

### Fase 4 — Central de Bíblia (1,5–2 semanas)
- Schema 4.2, importador de módulos `.bible`, ao menos 1 versão em domínio público incluída.
- Pesquisa por referência ("Jo 3:16-18"), parser de referências pt-BR com abreviações, pesquisa textual FTS.
- Sequências de versículos não consecutivos; snapshot do texto na programação.
- Tema próprio da Bíblia; exibição opcional de referência/versão/números.
- Favoritos e histórico de referências.

**Saída:** projetar leitura bíblica com versículos de livros diferentes na mesma sequência.

### Fase 5 — Central de Apresentação (2–3 semanas)
- Schema 4.3; importação de imagens e vídeos para a biblioteca de mídia (hash, thumbnails, localização por hash).
- Apresentações internas: slides com elementos texto/imagem/vídeo/cor; editor simples (posição, tamanho, camada, opacidade).
- Aviso rápido (texto sobre tema padrão sem abrir editor).
- Loop de apresentação (recepção/avisos).
- Pré-carregamento de mídias do próximo item.
- PowerPoint fica para V2 (importação como imagens via LibreOffice headless será avaliada — ADR).

**Saída:** culto com música + Bíblia + vídeo + aviso na mesma programação.

### Fase 6 — Central de Áudio completa (2 semanas)
- Playlist ordenável (drag & drop), seletor de arquivos, biblioteca + itens temporários de sessão.
- Dois decks + crossfade, modos automático/manual/loop faixa/loop playlist.
- Indicador TOCANDO grande e persistente + barra de progresso + próxima faixa.
- Volume geral e por faixa, cue in/out, fades configuráveis.
- Proteção contra reprodução simultânea; atalhos de vinhetas/efeitos.
- Histórico completo no EventStore.

**Saída:** todos os estados da tabela da Seção 8 do relatório funcionando e visíveis na UI.

### Fase 7 — Programação unificada e operação ao vivo (2 semanas)
- Programação misturando as quatro centrais; grupos/etapas do culto; notas internas; overrides por item.
- Snapshot autocontido ao salvar; exportar/abrir pacote `.service` (ZIP com banco + mídias).
- Item atual / próximo / histórico imediato na UI; salto livre sem perder estado do áudio.
- Duplicar, renomear, arquivar programações; autosave com versões de recuperação.
- Pré-validação "Verificar culto"; modo seguro; watchdog de mídia.

**Saída:** ensaio geral — culto completo simulado de 90 min sem intervenção técnica.

### Fase 8 — Remote Server + app mobile (3–4 semanas)
- `libremote` completo: mDNS, HTTP, WebSocket, pareamento SPAKE2, tokens, papéis, revogação.
- Especificação `docs/protocol/v1.md` congelada.
- App Flutter (iOS + Android): descoberta, pareamento, programação, controle de slides, player de áudio com TOCANDO, pesquisa.
- Reconexão automática; latência de comando < 200 ms na rede local.

**Saída:** operar um culto inteiro só pelo celular, com o desktop intocado.

### Fase 9 — Endurecimento e empacotamento (2 semanas)
- Assinatura e notarização (macOS), instalador assinado (Windows).
- Soaktest de 4 h no CI noturno passando 7 dias seguidos.
- Testes de campo: 2+ cultos reais acompanhados, bugs triados e corrigidos.
- Documentação do operador (guia rápido).
- Beta fechado.

**Saída do MVP:** instaladores das duas plataformas, checklist da Seção 16 do relatório 100 % atendido.

**Duração total estimada do MVP: ~18–24 semanas** de desenvolvimento contínuo. A integração do layout final (Google Stitches) acontece em paralelo: como a UI QML é desacoplada do Core (Seção 2), trocar o layout não altera lógica.

---

## 11. Fases — Versão 2 (detalhado)

### V2.1 — Retorno de palco (1,5 semanas)
- Novo papel de saída `stage` no Output Manager com layout próprio: letra atual, próxima letra, relógio, cronômetro do culto e recados do operador.
- Editor de layout de palco simples (quais blocos aparecem, tamanhos).
- Recado instantâneo para o palco enviado do desktop ou do mobile (papel `full`).

### V2.2 — Saída de transmissão com alpha (2 semanas)
- Render de texto/elementos com fundo transparente em janela dedicada (para captura pelo OBS via window capture).
- Modo "lower third": tema específico de transmissão com posição inferior.
- Sincronizado ao mesmo estado Live (nenhuma operação extra para o operador).

### V2.3 — Pré-escuta em dispositivo separado (1 semana)
- Deck de pré-escuta roteado a um segundo dispositivo de áudio (fone) via Audio Engine.
- Botão "ouvir" em qualquer faixa sem afetar a saída principal.

### V2.4 — Editor visual avançado e slides mestres (3 semanas)
- Elementos adicionais: forma, gradiente, relógio, contagem regressiva, campos dinâmicos (nome do culto, data, próxima etapa).
- Slides mestres com herança; alinhamento com guias e distribuição; animações de elemento (entrada/saída).
- Biblioteca de temas completa com pré-visualização.

### V2.5 — Importação de PowerPoint (2 semanas)
- Rota 1 (padrão): conversão via LibreOffice headless empacotado → slides como imagens de alta resolução.
- Rota 2 (melhor esforço): parsing OOXML de texto/imagens para slides editáveis quando a estrutura for simples.
- Relatório de fidelidade pós-importação (o que foi convertido como imagem vs editável).

### V2.6 — Atalhos, MIDI e Stream Deck (2 semanas)
- Mapa de atalhos configurável (todas as ações do CommandBus são mapeáveis — arquitetura já permite).
- Entrada MIDI (nota/CC → comando) via RtMidi.
- Integração Stream Deck (plugin oficial ou modo companion via WebSocket local usando o mesmo protocolo mobile).

### V2.7 — Sincronização opcional de bibliotecas e backups (3 semanas)
- Ativação da `sync_queue`: exportação/importação incremental entre computadores da mesma igreja (arquivo ou pasta compartilhada; sem nuvem obrigatória).
- Resolução de conflitos por `revision` + timestamps, com revisão manual quando ambíguo.
- Backup agendado para pasta/disco externo.

**Duração estimada V2: ~14–16 semanas.**

---

## 12. Fases — Versão Profissional (detalhado)

### Pro.1 — NDI com alpha e áudio (3 semanas)
- Integração NDI SDK (licenciamento próprio; ADR antes de iniciar).
- Cada saída pode ser espelhada/enviada como fluxo NDI (vídeo + alpha + áudio).
- Descoberta NDI na rede; painel de status de fluxos.

### Pro.2 — Múltiplas saídas independentes (2 semanas)
- N saídas com layout e conteúdo independentes (ex.: telão principal, TVs do hall com avisos, sala infantil).
- Roteamento por saída: qual central/conteúdo alimenta cada destino.

### Pro.3 — Feeds de câmera e captura (3 semanas)
- Entrada de dispositivos de captura (UVC/BlackMagic) e fluxos NDI como elemento de slide ou fundo.
- Latência monitorada; picture-in-picture no compositor.

### Pro.4 — Automação e API pública (3 semanas)
- Gatilhos: "ao entrar o item X, executar comandos Y" (ex.: iniciar contagem + vinheta).
- API pública = protocolo mobile promovido a estável e documentado publicamente (REST + WebSocket) com tokens de aplicação.
- Webhooks locais para integração com OBS/vMix/automação da igreja.

### Pro.5 — Multiusuário e permissões avançadas (2 semanas)
- Múltiplos operadores simultâneos (desktop + mobiles) com papéis granulares por central.
- Trava de edição por item; indicação de quem está operando o quê.

### Pro.6 — Telemetria local e diagnóstico (2 semanas)
- Painel de desempenho (fps por saída, buffer de áudio, latência mobile, memória).
- Exportação de diagnóstico completo autorizada; opção de sessão de suporte remoto por consentimento explícito.

**Duração estimada Pro: ~15 semanas.**

---

## 13. Riscos e mitigações

| # | Risco | Prob. | Impacto | Mitigação |
|---|---|---|---|---|
| 1 | Sincronização A/V ou fluidez de vídeo insuficiente em alguma GPU/driver Windows | Média | Alto | Fase 1 dedicada; matriz de teste com GPUs Intel/AMD/NVIDIA; fallback de decodificação por software |
| 2 | Ciclo de desenvolvimento C++ lento (builds, debugging) | Alta | Médio | Bibliotecas estáticas modulares, ccache/sccache, testes headless rápidos, presets Ninja |
| 3 | Multi-monitor com DPI misto no Windows | Média | Alto | Testes reais semanais desde Fase 3; janela de saída própria com escala manual controlada |
| 4 | LGPL: erro de empacotamento (linkagem estática acidental) | Baixa | Alto | Verificação automatizada no CI do artefato final (dependências dinâmicas listadas) |
| 5 | Direitos de versões bíblicas | Média | Médio | MVP só com domínio público; arquitetura de módulos licenciados desde o início |
| 6 | Layout final (Stitches) exigir reestruturação da UI | Média | Baixo | UI desacoplada do Core; QML provisório descartável por definição |
| 7 | Drift de comportamento macOS vs Windows | Média | Alto | CI dupla, testes de imagem de referência, PAL auditada, proibição de `#ifdef` fora do PAL |
| 8 | Glitches de áudio em máquinas fracas | Média | Alto | Callback lock-free, buffers ajustáveis, soaktest em hardware modesto |

---

## 14. Diretrizes para o agente de desenvolvimento (Opus)

1. **Ordem é lei:** seguir as fases; dentro de uma fase, priorizar caminho crítico (engine antes de UI).
2. **Definition of Done por entrega:** código + testes passando nas duas plataformas no CI + documentação mínima (README do módulo atualizado).
3. **Nunca no thread de áudio:** alocação, locks, I/O, SQLite, logging síncrono.
4. **Nunca no thread de UI:** decodificação, I/O de banco pesado, hashing de mídia (usar workers).
5. **Todo comando novo** entra em: enum do CommandBus, serialização JSON, validação de permissão, teste de contrato do protocolo.
6. **Nenhum texto em RTF** — conteúdo textual é sempre texto puro estruturado (lição explícita do relatório sobre o EasyWorship).
7. **Idioma:** UI em pt-BR com infraestrutura de i18n do Qt (`tr()`) desde o início; código, commits e docs técnicos em inglês.
8. **ADR obrigatório** para: troca de dependência, mudança de schema fora de migração planejada, desvio de fase.
9. **Commits pequenos e descritivos**; branch por fase; merge na main só com CI verde nas duas plataformas.
10. **Ao encontrar ambiguidade** entre este plano e o Relatório de Visão, o Relatório define *o quê*, este plano define *como*; conflitos reais → registrar ADR e perguntar ao idealizador.

---

## 15. Decisões em aberto (herdadas + novas)

Do relatório (Seção 18), permanecem abertas e **não bloqueiam o início**: nome do produto, organização visual das centrais, disposição de telas/botões (virá do Stitches), atalhos definitivos, modos visuais claro/escuro, modelo comercial, integração futura com IgrejAtiva.

Novas decisões técnicas a fechar durante o desenvolvimento (cada uma vira ADR):

- Formato de importação de letras da primeira versão (OpenLyrics? texto simples? ambos?).
- Quais versões bíblicas em domínio público incluir no instalador.
- LibreOffice headless embarcado (tamanho do instalador) vs conversão externa para PowerPoint na V2.
- MSIX vs Inno Setup no Windows.
- Nome do serviço mDNS definitivo (depende do nome do produto).

---

## 16. Checklist de prontidão para iniciar (Fase 0)

- [ ] Criar repositório Git (privado) com esta estrutura.
- [ ] Conta GitHub com Actions habilitado (runners macOS + Windows).
- [ ] Máquina/VM Windows com GPU disponível para testes manuais (a partir da Fase 3).
- [ ] Instalar no Mac de desenvolvimento: Xcode + CLT, Qt (última estável open-source), CMake ≥ 3.27, Ninja, vcpkg.
- [ ] Definir codinome provisório do produto (para bundle id, ex.: `br.com.<empresa>.projetor`).
- [ ] Aprovação deste plano pelo idealizador.

---

*Fim do plano técnico. Este documento deve ser mantido no repositório em `docs/plan/` e atualizado por ADRs, nunca reescrito silenciosamente.*
