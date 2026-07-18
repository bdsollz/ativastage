# Referência de UI — "Studio Precision" (Stitch / Lumina Worship Station)

Fonte: export do Google Stitch enviado pelo idealizador (zip `stitch_lumina_worship_station`).
Arquivos brutos preservados em `docs/design/stitch/` — cada tela tem `screen.png`
(mockup) e `code.html` (markup Tailwind exato, útil como referência pixel a pixel).
Design system completo em `docs/design/stitch/studio_precision/DESIGN.md`.

> **Papel no projeto:** este é o *layout visual final* que o Plano Técnico
> antecipa (Seção 2 / risco #6): a UI QML é desacoplada do Core, então o visual
> pode ser trocado sem mexer na lógica. Estas telas guiam a UI a partir da
> **Fase 3** (a UI provisória da Fase 3 evolui para este visual). NÃO são
> necessárias na Fase 1/2 (núcleo/engines).

---

## 1. Princípios (para sala de controle escura)

- Estética **High-Contrast Corporate Modern**: base monocromática bem escura
  (mínimo de "light spill"), acentos de alta croma só para estados críticos.
- Profundidade por **camadas tonais + bordas de 1px**, NÃO por sombras.
- Densidade: "Comfortable" em telas de setup, "Compact" no ao vivo.
- Contraste mínimo 7:1 para labels de status.
- Todo texto técnico/numérico (timecode, hex, coordenadas) em **JetBrains Mono**.

## 2. Tokens essenciais

**Fontes:** Inter (UI) + JetBrains Mono (números/timecode).

**Cores de estado (semântica — memorizar):**

| Estado | Cor | Uso |
|---|---|---|
| LIVE / no ar / destrutivo | Vermelho `#EF4444` | botão GO LIVE, chip ●LIVE, BLACK/EMERGENCY; pulsa (opacity 0.8→1.0) quando no ar |
| PREVIEW / preparado | Âmbar `#F59E0B` / `#ffb95f` | conteúdo montado ainda não ao vivo, chip NEXT |
| ACTIVE / seleção / foco | Azul `#3B82F6` / primary `#adc6ff` | seleção, borda de foco, barra vertical 4px na lista |
| SUCCESS / tocando | Verde `#10B981` | playback ativo (ícone play fica verde, chip PLAYING/TOCANDO) |

**Superfícies (tonal layering):**
`background #10131a` → painel `#1d2027`/#171717 com borda 1px `#424754`/#262626
→ popovers `#272a31`/#262626 com blur 8px e borda mais clara.
Input mais escuro que a superfície, borda 1px, foco azul.

**Tipografia (px):** display-lg 48/700 · headline-lg 32/600 · headline-md 24/600
· title-lg 20/600 · title-md 16/600 · body-lg 16/400 · body-md 14/400 ·
label-md 12/600 (UPPERCASE, tracking 0.05em) · label-sm 10/700 (tracking 0.08em)
· mono-md 14/400 (JetBrains Mono). Mobile: reduzir display 20%, mínimo 14px no corpo.

**Raio:** sm 4px · default 8px (botões/inputs/painéis pequenos) · md 12px · lg 16px
(áreas grandes só quando flutuantes; docked = cantos retos p/ ganhar espaço) · full.

**Espaçamento:** unidade base 4px · gutter 16px · margem de página 24px ·
padding de painel 12px · gap de pilha 8px. Grid 12 col ≥1280px, 8 col 768–1279px
(painéis secundários viram drawers), 1 col <768px (foco em transporte + Blackout).

**Componentes-chave:** botão primário azul sólido; botão Live vermelho pulsante;
botão ghost só borda; chips de status (ex.: "4K" branco sobre cinza `#404040`);
transport controls grandes (play fica verde no ativo); cards de mídia 16:9 com
barra de progresso embaixo; listas densas com divisor 1px e barra azul 4px à
esquerda no selecionado; VU meters em gradiente verde→âmbar→vermelho.

## 3. Shell global (comum a quase todas as telas)

- **Sidebar esquerda:** marca ("ProChurch AV" — nome provisório), card do
  usuário/local ("Main Sanctuary • On Air"), botão grande **GO LIVE** (vermelho),
  navegação: **Music · Bible · Presentation · Audio · Library · Settings**; rodapé
  Help · Status. Item ativo com barra/realce azul. (Nota: Audio Central aparece
  também numa variante com nav no TOPO — ver §4.3.)
- **Header:** título da tela + busca; ícones utilitários à direita (histórico,
  atalhos, temas/layout, settings).
- **Barra de transporte inferior (global):** **PREV · NEXT · CLEAR · BLACK ·
  LOGO · EMERGENCY**. Mapear direto aos comandos globais do CommandBus
  (`NextSlide`, `PrevSlide`, `ClearText/ClearAll`, `ShowBlack`, `ShowLogo`).

## 4. Telas (5 referências)

### 4.1 Music Central — `prochurch_av_central_de_m_sica`
Layout 3 colunas: **(a)** biblioteca com busca + tabs "All Songs/Favorites/Recent"
+ lista de músicas (título, artista, tom, botão play); **(b)** sequência do item
selecionado — cabeçalho (título, arranjo, tom, BPM, botão "Edit Sequence") e
blocos semânticos coloridos por seção (INTRO âmbar, VERSE azul c/ chip PREVIEW,
CHORUS vermelho, VERSE 2…); **(c)** **LIVE OUTPUT** — grade de miniaturas dos
slides com chips ●LIVE (borda azul) e NEXT (borda âmbar), label por slide
("V1 - Slide 1 · Active"); toggle grade/lista no canto.

### 4.2 Bible Central — `prochurch_av_central_da_b_blia`
3 colunas: **(a)** "Books" em grid 2-col com toggle OT/NT + painel "Chapter"
(números) embaixo; **(b)** versículos do capítulo selecionado (nº + texto,
selecionado com barra azul), botão "Select All"; **(c)** **Preview** 16:9 com o
versículo renderizado + referência (ex.: "Psalms 23:1 NIV"), botão **SEND LIVE**
(azul), abas **History/Favorites**. Header: busca por referência/keywords +
seletor de versão (ex.: "NIV").

### 4.3 Audio Central — `prochurch_av_central_de_udio` (e variante `_layout_alinhado`)
Central de áudio com **decks**: **Main Deck** (título/artista, waveform,
timecodes decorrido/-restante, transport grande com play verde, Fade Out,
volume) marcado ●PLAYING/TOCANDO + seletor de **Output** (ex.: "FOH L/R (Dante
1-2)"); **Pre-listen / pré-escuta** (READY, OUT: HP MIX, CUE); **Fila de
reprodução** (POS, TÍTULO, ARTISTA, DUR., RESTANTE, STATUS [TOCANDO/CARREGADA/
AGUARDANDO], VOL.) com drag-and-drop e AUTO-PLAY/LIMPAR FILA; **Soundboard** de
disparos rápidos (vinhetas/efeitos) na variante alinhada; **Biblioteca de
arquivos** com categorias Músicas/Instrumentais/Fundos/Vinhetas/Efeitos/Avisos/
Playbacks/Trilhas. Header da variante: "AUDIO CENTRAL V3.2 · SYS OK · DANTE SYNC
· CPU 12%" + **MUTE GERAL** (vermelho). Arquivo ausente aparece riscado em vermelho.

### 4.4 Saídas e Hardware — `prochurch_av_sa_das_e_hardware`
"Settings / Output Routing". **Video & Display Routing:** cards por saída com
preview, chip de status (CONNECTED/ACTIVE BROADCAST/WAITING FOR CLIENT), toggle
on/off, e metadados (Public Screen → GPU Port DisplayPort 1; Stage Display →
HDMI 1; NDI Stream → target/IP + TX Mbps; Virtual Camera → Zoom/Teams). Botão
"+ ADD OUTPUT". **Audio Routing:** interfaces (ex.: "Main Audio Out · ASIO:
Focusrite USB ASIO") com VU meter + SETUP. Mapear aos papéis de saída do
Output Manager (público/palco/transmissão) e ao IScreenService/IAudioDeviceService.

## 5. Terminologia pt-BR observada (usar no i18n)

TOCANDO AGORA · FILA DE REPRODUÇÃO · MUTE GERAL · LIMPAR FILA · AUTO-PLAY ·
DECORRIDO / TOTAL · RESTANTE · CARREGADA · AGUARDANDO · Arquivo Ausente ·
Músicas/Instrumentais/Fundos/Vinhetas/Efeitos/Avisos/Playbacks/Trilhas ·
SOUNDBOARD (Disparos Rápidos) · ADICIONAR · PASTAS. (UI final em pt-BR; a marca
"ProChurch AV" é placeholder — nome do produto ainda em aberto.)

## 6. Como usar ao construir a UI (QML)

- Traduzir os tokens (§2) para um **Theme singleton QML** (cores, fontes,
  espaçamentos, raios) — fonte única de estilo; nunca hardcodar hex nas telas.
- Consultar `code.html` de cada tela para medidas/estrutura exatas; consultar
  `screen.png` para o resultado visual.
- Manter o mapeamento **UI → CommandBus** (a barra de transporte e ações Live/
  Preview já correspondem a comandos do Core). Preview e Live são estados
  distintos: só SEND LIVE / GO LIVE promovem Preview → Live.
- Ordem de implementação segue as fases: Music (F3), Bible (F4), Presentation
  (F5), Audio (F6), Saídas/unificação (F7).
