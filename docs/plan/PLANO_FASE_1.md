# PLANO DE IMPLEMENTAÇÃO — FASE 1 (Prova audiovisual) ⚠

> Fase de risco. Objetivo: **provar o núcleo audiovisual difícil antes de qualquer
> UI ou biblioteca**. Se algo estrutural falhar aqui, o projeto para para reavaliação
> (gate da Seção 10 do plano técnico). Nada de música/bíblia/apresentação foi
> construído — e não deve ser, até este gate passar.

- **Pré-requisito:** Fase 0 concluída (CI verde nas 2 plataformas). ✅
- **Fontes:** `docs/plan/Plano_Tecnico_Desenvolvimento.md` §5 (núcleo A/V), §1 (stack),
  §2 (arquitetura), §8 (macOS→Windows), §13 (riscos); `ESTADO_ATUAL.md`.
- **Duração estimada:** 2–3 semanas.
- **Branch sugerida:** `phase/1-av-proof` (criada a partir de `phase/0-foundation`).

---

## 1. Critérios de saída da fase (o "pronto" da Fase 1)

A fase só é considerada concluída quando **todos** os itens abaixo passam nas **duas
plataformas** (macOS de desenvolvimento + Windows real/VM com GPU):

1. Janela de saída fullscreen, sem moldura, em monitor secundário — mac e win.
2. Vídeo H.264 **1080p e 4K** tocando fluido via FFmpeg → textura RHI, com áudio
   sincronizado (clock mestre = áudio).
3. Pré-carregamento (`Primed`) com **GoLive < 100 ms** sem tela preta.
4. Loop de vídeo sem "pisca".
5. Audio Engine mínimo: 1 deck com play/pause/stop/volume/fade-out, dispositivo
   de saída selecionável.
6. Hot-plug: desconectar o monitor de saída durante a reprodução **não derruba o app**
   (saída pública cai para modo janela/ensaio).
7. **Soaktest de 1 h** rodando nas duas plataformas sem vazamento de memória, sem
   travar decoder, sem drift de A/V acumulado.
8. Demo gravada nas duas plataformas + métricas de performance documentadas em
   `docs/perf/fase1.md`.

**Gate de decisão:** problemas estruturais (sync A/V inviável em alguma GPU/driver,
zero-copy impossível, latência de GoLive intransponível) **param o projeto** para
reavaliar arquitetura antes de investir em UI/bibliotecas.

---

## 2. Módulos criados nesta fase

| Módulo | Lib CMake | Responsabilidade nesta fase | Depende de |
|---|---|---|---|
| Media Engine | `libmedia` | Decode FFmpeg, preload/Primed, clock A/V, upload de frame p/ GPU | `libpal` |
| Audio Engine | `libaudio` | 1 deck: play/pause/stop/volume/fade; device select (miniaudio) | `libpal` |
| Output Manager | `liboutput` | `QQuickWindow` fullscreen por monitor; reatribuição no hot-plug | `libpal`, Qt Quick |
| App | `app` | Composição, QML mínimo de teste, fiação dos engines | todos acima |

Ampliações do **`libpal`** exigidas pela Fase 1:
- `IAudioDeviceService` — enumeração/seleção/hot-swap de dispositivo de áudio (nova).
- `IScreenService` — já existe; a Fase 1 **exerce de verdade** o hot-plug (sinal de
  conexão/desconexão) que hoje só está declarado.

**Invariantes reforçadas** (ver `ESTADO_ATUAL.md` §"Invariantes ativas"):
- `Engines` (`libmedia`/`libaudio`/`liboutput`) **nunca** dependem de UI.
- `Core` ainda não existe; engines expõem interfaces C++ puras + sinais Qt.
- Nenhum `#ifdef Q_OS_*` fora de `libpal` (HW decode e device audio entram via PAL/backend, não por ifdef espalhado).
- **Nada de alocação/lock/I/O/SQLite no thread de áudio** (miniaudio callback).
- Clock mestre = áudio; vídeo segura/descarta frames.

---

## 3. Ordem de trabalho (marcos incrementais)

Cada marco é **compilável e verificável isoladamente**. Ordem escolhida para reduzir
risco: primeiro o que destrava tudo (app + deps), depois o subsistema mais isolado
(áudio, testável headless), depois o mais arriscado (vídeo→GPU), por fim integração
A/V e robustez.

### Marco 1.0 — Reintroduzir dependências e alvo `app` mínimo
**Meta:** `ATIVASTAGE_BUILD_APP=ON` compila um executável Qt Quick que abre uma
janela vazia, nas duas plataformas, com FFmpeg e miniaudio disponíveis no build.

- Reintroduzir no `vcpkg.json`: `ffmpeg` (features mínimas: `avcodec`, `avformat`,
  `avfilter`, `swscale`, `swresample`; **sem** `--enable-gpl`, LGPL, só decode) e
  `miniaudio` (ou vendorizar o header único). `libsodium` **fica de fora** até o
  Remote (Fase 8) — anotar.
- Criar `src/app/CMakeLists.txt`: alvo `app` via `qt_add_executable` +
  `qt_add_qml_module`; QML mínimo (`Main.qml` com janela vazia + label "AtivaStage").
- Ligar `ATIVASTAGE_BUILD_APP=ON` nos presets `mac-*`/`win-*` (manter OFF no
  preset "leve" se ainda existir).
- `AtivaStage.command`: a partir daqui monta o `.app` com `macdeployqt` (já previsto
  no ADR-0002; ativar o caminho do bundle).
- Atualizar CI: reabilitar Qt no job de build (setup do Qt via `jurplel/install-qt-action`
  ou aqua/vcpkg), aceitar o custo do build de ffmpeg (cache do vcpkg no runner).

**Aceite:** `app` abre janela vazia em mac e win; CI verde nas duas plataformas com
ffmpeg+miniaudio resolvidos. Nenhum decode ainda.

**Riscos:** tempo de build do ffmpeg no CI (mitigar com cache binário do vcpkg / features
mínimas). Se estourar tempo do runner, considerar ffmpeg pré-buildado por plataforma.

---

### Marco 1.1 — PAL de áudio + Audio Engine (1 deck)
**Meta:** tocar um WAV/MP3 num dispositivo selecionável, com play/pause/stop/volume/fade,
sem GUI (teste headless).

- `libpal`: `IAudioDeviceService` (enumerar dispositivos, default, selecionar, sinal de
  remoção/hot-swap). Implementação via miniaudio context (é multiplataforma; **sem
  ifdef** — miniaudio já abstrai WASAPI/CoreAudio). Fica em `libpal` por ser fronteira
  de plataforma.
- `libaudio`: classe `AudioEngine` com **1 `Deck`**:
  - Estados exatos do relatório: `PRONTO`, `CARREGANDO`, `TOCANDO`, `PAUSADO`,
    `FADE OUT`, `FALHA`.
  - API: `load(path)`, `play()`, `pause()`, `stop()`, `setVolume(db)`, `fadeOut(ms)`.
  - Decodificação do arquivo de áudio via FFmpeg (`swresample` para PCM float) →
    ring buffer; callback miniaudio **só lê** do ring buffer e aplica ganho/rampa.
  - **Callback de tempo real:** sem malloc, sem lock, sem I/O. Comunicação por ring
    buffer + `std::atomic` (comandos e volume alvo). Fade = rampa calculada no callback.
- Fornece o **clock de áudio** (amostras reproduzidas → tempo) que a Fase 1.3 usará
  como clock mestre de A/V.

**Aceite (headless):** teste Catch2/CLI que carrega um WAV curto e verifica transições
de estado; teste manual que toca áudio e troca de dispositivo ao vivo sem crash.
**Verificação de invariante:** revisão do callback confirmando ausência de alloc/lock
(pode-se instrumentar com um assert de debug que falha se `malloc` for chamado no
thread de áudio).

**Riscos:** seleção/hot-swap de dispositivo difere sutilmente entre WASAPI e CoreAudio
(miniaudio cobre, mas testar remoção do device ativo). Glitches por ring buffer mal
dimensionado — medir underruns.

---

### Marco 1.2 — Output Manager + janela fullscreen em monitor secundário
**Meta:** abrir uma `QQuickWindow` sem moldura, fullscreen, no monitor designado, nas
duas plataformas; sem monitor secundário, cai para janela flutuante (modo ensaio).

- `liboutput`: `OutputManager` que consulta `IScreenService` (PAL) e cria uma
  `OutputWindow` (`QQuickWindow`/`Window` QML sem moldura) por saída física.
- Mapeamento persistente monitor↔papel (mínimo nesta fase: só "público"); posicionar
  no monitor secundário; fullscreen real (testar comportamento de fullscreen
  multi-monitor no Windows, que difere do mac).
- Conteúdo de teste: um retângulo colorido / imagem estática + os comandos globais
  mínimos `ShowBlack` e `ClearAll` (o resto vem na Fase 3).
- **DPI/escala:** validar escala manual controlada (Windows mistura DPI por monitor).

**Aceite:** janela fullscreen aparece no monitor secundário em mac e win; sem 2º
monitor, vira janela redimensionável. Sem crash ao alternar.

**Riscos:** fullscreen multi-monitor + DPI misto no Windows (risco §13-3 do plano).
Documentar diferenças observadas.

---

### Marco 1.3 — Media Engine: decode → textura RHI + sync A/V
**Meta:** o coração do risco. Tocar H.264 1080p **e** 4K fluido, vídeo em textura GPU,
áudio sincronizado (clock mestre = áudio).

- `libmedia`: `MediaEngine` + `VideoDecoder`:
  - Thread de demux/decode por mídia: `avformat` → filas de pacotes → `avcodec`.
  - **Aceleração de hardware:** VideoToolbox (macOS), D3D11VA (Windows), **fallback
    software**. A escolha do hwaccel é fronteira de plataforma → factory em `libpal`
    ou tabela interna sem `#ifdef Q_OS_*` espalhado (usar capabilities do FFmpeg).
  - Frame de vídeo → textura do scene graph via `QSGVideoNode`/textura RHI externa;
    **zero-cópia** quando o decoder de HW permitir (mapear surface → textura RHI).
    Fallback: `swscale` para RGBA + upload de textura.
  - **Sync:** clock mestre = áudio (do Marco 1.1). Vídeo apresenta o frame cujo PTS
    casa com o clock de áudio; segura ou descarta frames para não driftar. Vídeo sem
    áudio usa clock monotônico.
  - Integração com a `OutputWindow` (1.2): o vídeo renderiza na saída pública.

**Aceite:** clipe 1080p e clipe 4K tocam fluidos (medir frames apresentados vs
descartados, jitter de apresentação); A/V em sincronia perceptível e medida (offset
< ~40 ms). Sem tearing/travamento visível. Registrar caminho HW vs software usado.

**Riscos (o maior da fase):** zero-copy HW→RHI é a parte frágil, especialmente no
Windows com GPUs Intel/AMD/NVIDIA distintas (risco §13-1). Ter **fallback software
funcionando primeiro**, depois otimizar para HW. Matriz de GPUs no Windows.

---

### Marco 1.4 — Preload (`Primed`) + GoLive < 100 ms + loop sem pisca
**Meta:** transição para o ar instantânea, sem tela preta; loop contínuo limpo.

- Estado `Primed`: ao entrar em "preview", o engine abre o arquivo, decodifica os
  primeiros ~500 ms e **pausa pronto** (primeiro frame já na GPU).
- `GoLive`: apresenta o frame já pronto e solta o clock em **< 100 ms**, sem tela preta.
- Loop: seek para 0 reaproveitando frame decodificado, sem "pisca" na virada.

**Aceite:** medir latência GoLive (timestamp do comando → primeiro frame no ar) < 100 ms
em mac e win; loop de clipe curto rodando N ciclos sem flash na emenda.

**Riscos:** seek preciso + primeira imagem sem preto exige cuidado com keyframes/GOP;
testar clipes com GOP longo.

---

### Marco 1.5 — Robustez: hot-plug de monitor + watchdog de decoder
**Meta:** o app sobrevive a eventos hostis durante a reprodução.

- Hot-plug: desconectar o monitor de saída durante a reprodução dispara o sinal do
  `IScreenService`; o `OutputManager` reatribui a saída (cai para janela/ensaio) **sem
  derrubar** o pipeline de mídia/áudio.
- Watchdog de mídia (mínimo): decoder travado > 2 s → mata e reinicia o pipeline
  daquela mídia mostrando fundo (nunca tela preta congelada).
- Hot-swap de dispositivo de áudio (do 1.1) revalidado sob reprodução.

**Aceite:** roteiro manual — tocar vídeo, puxar o cabo do 2º monitor, reconectar;
trocar device de áudio ao vivo; app continua. Nas duas plataformas.

---

### Marco 1.6 — Soaktest 1 h + métricas + demo
**Meta:** provar estabilidade sob duração e coletar evidências para o gate.

- Cenário de soak: playlist de vídeos (1080p + 4K) em loop por ≥ 1 h, áudio tocando,
  medindo: memória (sem crescimento monotônico = sem leak), FPS/frames descartados,
  drift de A/V acumulado, uso de CPU/GPU, underruns de áudio.
- Rodar nas **duas plataformas**. Automatizável parcialmente (headless para áudio/decode;
  vídeo exige janela).
- `docs/perf/fase1.md`: tabela de métricas por plataforma/GPU + caminho HW/software.
- Gravar a demo (mac + win).

**Aceite:** 1 h sem crash, sem leak, sem travar decoder, drift estável. Métricas
documentadas. **→ Gate de decisão da Fase 1.**

---

## 4. Dependências entre marcos

```
1.0 (app+deps) ──┬──► 1.1 (áudio)  ─────────────┐
                 ├──► 1.2 (output) ──► 1.3 (vídeo→GPU+sync) ──► 1.4 (preload/golive)
                 │                          ▲                        │
                 │        (clock de áudio) ─┘                        ▼
                 └────────────────────────────────────────► 1.5 (robustez) ──► 1.6 (soak+gate)
```

1.1 e 1.2 são paralelizáveis após 1.0. 1.3 precisa de 1.1 (clock) e 1.2 (janela).

---

## 5. Matriz de teste de hardware (manual, obrigatória para o gate)

| Item | macOS (dev) | Windows (real/VM + GPU) |
|---|---|---|
| Fullscreen em 2º monitor | ✔ | ✔ (fullscreen multi-mon difere) |
| DPI/escala misto | n/a habitual | ✔ (Windows mistura escalas) |
| HW decode | VideoToolbox | D3D11VA — testar Intel/AMD/NVIDIA |
| Fallback software | ✔ | ✔ |
| Hot-plug do monitor | ✔ | ✔ (HDMI) |
| Hot-swap de áudio | CoreAudio | WASAPI |
| Soak 1 h | ✔ | ✔ |

**Bloqueio conhecido (herdado do checkpoint):** máquina/VM Windows com GPU para os
testes manuais. Marcos 1.0–1.1 (e o headless de 1.3 em software) avançam sem ela; os
critérios de saída de vídeo/fullscreen/hot-plug **exigem** o Windows real antes do gate.

---

## 6. Itens transversais

- **vcpkg baseline/pin:** ao reintroduzir ffmpeg, fixar baseline do vcpkg (decisão em
  aberto do checkpoint) para builds reproduzíveis. Registrar em ADR.
- **CI:** custo do build sobe (ffmpeg). Usar cache binário do vcpkg; se necessário,
  separar job "leve" (libs+testes headless) de job "app" (Qt+ffmpeg) para feedback rápido.
- **ADRs novos previstos:** ADR-000x "Aceleração de vídeo por plataforma e estratégia
  zero-copy"; ADR-000x "miniaudio como backend de áudio e contrato do callback RT";
  ADR-000x "baseline do vcpkg com ffmpeg". (Schema não muda nesta fase → sem migração.)
- **UI:** a referência visual do Stitch (`docs/design/`) **não** é usada aqui. QML da
  Fase 1 é utilitário/descartável. A UI real começa na Fase 3.
- **i18n:** textos de UI em pt-BR via `tr()` mesmo no QML utilitário; código/commits/docs
  técnicos em inglês (invariante).

---

## 7. Primeiro passo concreto ao retomar

Começar pelo **Marco 1.0**: reintroduzir `ffmpeg`+`miniaudio` no `vcpkg.json`, criar
`src/app/CMakeLists.txt` com um Qt Quick abrindo janela vazia, ligar `ATIVASTAGE_BUILD_APP`
nos presets e reabilitar Qt no CI. Validar CI verde nas duas plataformas **antes** de
tocar em qualquer decode. Só então seguir para 1.1.
