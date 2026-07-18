# Método "auto-compilar por duplo clique" — como funciona e como reusar

Guia para levar o mesmo esquema do Context para outro app. A ideia central: em
vez de distribuir um binário pronto (que exige conta paga de desenvolvedor,
assinatura e notarização), **você distribui o código-fonte + um script `.command`
de duplo clique** que, na máquina do usuário, instala o que precisa, **compila**,
monta o `.app` e instala. Rodar o mesmo script de novo = **atualizar**.

## Por que funciona sem conta de desenvolvedor

O macOS deixa você criar e rodar um `.app` compilado localmente sem assinatura
da Apple. O único obstáculo é a "quarentena" (arquivos vindos da internet), que o
próprio script remove com `xattr`. Não há App Store, TestFlight nem notarização
no caminho — é instalação local pura. (No iPhone é diferente: exige o Xcode e uma
conta Apple grátis para assinar; ver a seção do iOS.)

## As três peças

1. **O script instalador/atualizador** (`SeuApp.command`) — o coração do método.
2. **O projeto compilável** — um Swift Package executável (`Package.swift` +
   `Sources/`), mais um arquivo `VERSION` e, opcional, `icon.png`/`splash.png`.
3. **(Opcional) Auto-atualização de dentro do app** — o app compara o `VERSION`
   da pasta de código com a versão dele e oferece um botão "Atualizar".

## Peça 1 — o script `.command` (o que ele faz, em ordem)

Sempre os mesmos passos, e todos **idempotentes** (rodar de novo não quebra):

1. `cd "$(dirname "$0")"` — trabalha a partir da pasta onde o script está.
2. **Tira a quarentena** da pasta (`xattr -dr com.apple.quarantine`) para o macOS
   não bloquear.
3. **Garante as Ferramentas de Linha de Comando da Apple** (`xcode-select -p`); se
   faltarem, dispara `xcode-select --install` e pede para rodar de novo depois.
4. **Garante o Homebrew** e as **dependências** do app (ex.: `ffmpeg`), instalando
   só o que faltar e atualizando o resto.
5. **Baixa artefatos grandes** uma vez só (modelos, etc.), se o app usar.
6. **Compila**: `swift build -c release` e pega o binário com
   `swift build -c release --show-bin-path`.
7. **Monta o `.app`**: cria a estrutura `Contents/MacOS`, `Contents/Resources`,
   copia o binário, gera o `Info.plist` (com a versão lida do arquivo `VERSION`) e,
   se houver `icon.png`, gera o `.icns` com `sips` + `iconutil`.
8. **Instala em `/Applications`** (apaga o antigo e recria) e **abre** o app.
9. Detecta **instalação x atualização** simplesmente checando se o `.app` já existia.

O usuário dá dois cliques uma vez para instalar; depois abre pelo ícone. Quando
chega uma versão nova do código, dá dois cliques de novo = atualiza.

## Template pronto para adaptar

Troque `SeuApp` pelo nome do app e ajuste a lista de dependências e o
`CFBundleIdentifier`. Salve como `SeuApp.command` na raiz da pasta de código e
marque como executável (`chmod +x SeuApp.command`).

```bash
#!/bin/bash
# SeuApp — instalador/atualizador de 1 clique. Dois cliques: instala e abre.
# Para ATUALIZAR: dois cliques de novo quando chegar versão nova.
cd "$(dirname "$0")"
APPNAME="SeuApp"
BUNDLE_ID="com.suaempresa.seuapp"
DEPS=(ffmpeg)          # dependências do Homebrew (ajuste ou deixe vazio)

echo "== $APPNAME — instalando/atualizando =="
xattr -dr com.apple.quarantine "$(pwd)" 2>/dev/null || true

# 1) Ferramentas de desenvolvedor da Apple (necessárias para compilar)
if ! xcode-select -p >/dev/null 2>&1; then
  echo "→ Instalando as Ferramentas de Desenvolvedor da Apple…"
  xcode-select --install >/dev/null 2>&1 || true
  echo "  Quando terminar, feche e dê dois cliques aqui de novo."
  read -r -p "Enter para fechar…" _; exit 0
fi

# 2) Homebrew + dependências
if ! command -v brew >/dev/null 2>&1; then
  echo "→ Instalando o Homebrew (pode pedir a senha do Mac)…"
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi
[ -x /opt/homebrew/bin/brew ] && eval "$(/opt/homebrew/bin/brew shellenv)"
[ -x /usr/local/bin/brew ]    && eval "$(/usr/local/bin/brew shellenv)"
for dep in "${DEPS[@]}"; do
  brew list "$dep" >/dev/null 2>&1 || brew install "$dep"
done

# 3) Compilar
echo "→ Compilando…"
swift build -c release || { echo "!! Erro ao compilar."; read -r -p "Enter…" _; exit 1; }
BIN="$(swift build -c release --show-bin-path)/$APPNAME"

# 4) Montar o .app e instalar
VERSION="$(cat VERSION 2>/dev/null || echo 1.0.0)"
APP="/Applications/$APPNAME.app"
EXISTED="não"; [ -d "$APP" ] && EXISTED="sim"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp "$BIN" "$APP/Contents/MacOS/$APPNAME"; chmod +x "$APP/Contents/MacOS/$APPNAME"

ICON_LINE=""
if [ -f icon.png ] && command -v sips >/dev/null && command -v iconutil >/dev/null; then
  SET="$(mktemp -d)/$APPNAME.iconset"; mkdir -p "$SET"
  for SZ in 16 32 64 128 256 512 1024; do
    sips -z $SZ $SZ icon.png --out "$SET/icon_${SZ}x${SZ}.png" >/dev/null 2>&1
  done
  cp "$SET/icon_32x32.png"     "$SET/icon_16x16@2x.png"   2>/dev/null
  cp "$SET/icon_64x64.png"     "$SET/icon_32x32@2x.png"   2>/dev/null
  cp "$SET/icon_256x256.png"   "$SET/icon_128x128@2x.png" 2>/dev/null
  cp "$SET/icon_512x512.png"   "$SET/icon_256x256@2x.png" 2>/dev/null
  cp "$SET/icon_1024x1024.png" "$SET/icon_512x512@2x.png" 2>/dev/null
  iconutil -c icns "$SET" -o "$APP/Contents/Resources/$APPNAME.icns" >/dev/null 2>&1 \
    && ICON_LINE="<key>CFBundleIconFile</key><string>$APPNAME</string>"
fi

cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>$APPNAME</string>
  <key>CFBundleExecutable</key><string>$APPNAME</string>
  ${ICON_LINE}
  <key>CFBundleIdentifier</key><string>$BUNDLE_ID</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>${VERSION}</string>
  <key>CFBundleVersion</key><string>${VERSION}</string>
  <key>LSMinimumSystemVersion</key><string>13.0</string>
  <key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST

xattr -dr com.apple.quarantine "$APP" 2>/dev/null || true
[ "$EXISTED" = sim ] && echo "✅ $APPNAME atualizado ($VERSION)!" || echo "✅ $APPNAME $VERSION instalado!"
open "$APP"
```

O `Package.swift` correspondente é um executável simples:

```swift
// swift-tools-version: 5.9
import PackageDescription
let package = Package(
    name: "SeuApp",
    platforms: [.macOS(.v13)],
    targets: [.executableTarget(name: "SeuApp", path: "Sources/SeuApp")]
)
```

## Peça 3 — auto-atualização de dentro do app (opcional)

No Context, o app lê o arquivo `VERSION` da pasta de código e compara com a
versão do próprio bundle. Se a pasta estiver mais nova, aparece um botão
**"Atualizar"** que:

1. roda `swift build -c release` na pasta (app aberto, progresso ao vivo);
2. grava um script auxiliar em `/tmp` e o dispara **destacado** (`nohup`);
3. o app se fecha; o script espera o processo morrer, **remonta** o `.app`
   (os mesmos passos 4 do template) e reabre.

É conveniência: mesmo sem isso, o `.command` sozinho já instala e atualiza.

## Variante iOS (instalar no aparelho pelo cabo)

Para iPhone/iPad o esquema muda porque exige o Xcode e assinatura (conta Apple
grátis serve). O `instalar-no-iphone.command` faz:

1. confere o Xcode; instala o **XcodeGen** pelo Homebrew se faltar;
2. gera o `.xcodeproj` a partir de um `project.yml` (`xcodegen generate`) — assim
   você versiona só o `project.yml`, nunca o projeto gerado;
3. acha o aparelho no cabo com `xcrun devicectl list devices`;
4. compila com `xcodebuild` e instala com `xcrun devicectl device install app`.

A versão do iOS fica no `project.yml` (`MARKETING_VERSION` / `CURRENT_PROJECT_VERSION`),
e você sobe o número a cada envio.

## Regras que fazem o método não dar dor de cabeça

- **Uma cópia só do código, editada na raiz.** Nada de zips/duplicatas — geram
  builds desatualizadas.
- **Todo passo de setup/deploy vira um `.command` de duplo clique.** O usuário
  nunca cola comando no terminal.
- **Tudo idempotente:** instala só o que falta, atualiza o resto, roda quantas
  vezes quiser.
- **Segurança:** rodar o app a partir de `~/Downloads` é arriscado (é onde outros
  programas gravam arquivos, e a pasta é "vigiada" pela auto-atualização). Prefira
  `~/SeuApp`. E mantenha dependências como o `ffmpeg` atualizadas — arquivos
  malformados podem explorar versões antigas.
- **`VERSION` é a fonte da verdade** para instalar-vs-atualizar e para a
  auto-atualização interna.
