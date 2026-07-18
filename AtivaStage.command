#!/bin/bash
# AtivaStage — instalador/atualizador de 1 clique (macOS).
# Dois cliques: instala dependências, compila e (a partir da Fase 1) monta o .app.
# Para ATUALIZAR: dois cliques de novo quando chegar código novo.
#
# Método adaptado de METODO_AUTO_COMPILAR.md para a stack Qt 6 + CMake.
# Ver docs/decisions/ADR-0002-auto-compilar-qt.md.
set -o pipefail
cd "$(dirname "$0")" || exit 1

APPNAME="AtivaStage"
BUNDLE_ID="br.com.ativa.ativastage"
DEPS=(cmake ninja qt ffmpeg sqlite libsodium catch2 pkg-config)
BUILD_DIR="build/local-release"

echo "== $APPNAME — instalando/atualizando =="

# Tira a quarentena para o macOS não bloquear.
xattr -dr com.apple.quarantine "$(pwd)" 2>/dev/null || true

# 1) Ferramentas de Linha de Comando da Apple (necessárias para compilar).
if ! xcode-select -p >/dev/null 2>&1; then
  echo "→ Instalando as Ferramentas de Desenvolvedor da Apple…"
  xcode-select --install >/dev/null 2>&1 || true
  echo "  Quando terminar, feche esta janela e dê dois cliques aqui de novo."
  read -r -p "Enter para fechar…" _; exit 0
fi

# 2) Homebrew + dependências.
if ! command -v brew >/dev/null 2>&1; then
  echo "→ Instalando o Homebrew (pode pedir a senha do Mac)…"
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)" || {
    echo "!! Falha ao instalar o Homebrew."; read -r -p "Enter…" _; exit 1; }
fi
[ -x /opt/homebrew/bin/brew ] && eval "$(/opt/homebrew/bin/brew shellenv)"
[ -x /usr/local/bin/brew ]    && eval "$(/usr/local/bin/brew shellenv)"

echo "→ Verificando dependências (${DEPS[*]})…"
for dep in "${DEPS[@]}"; do
  brew list "$dep" >/dev/null 2>&1 || brew install "$dep"
done

QT_PREFIX="$(brew --prefix qt 2>/dev/null)"

# 3) Configurar + compilar (Homebrew local; vcpkg fica reservado ao CI).
echo "→ Configurando (CMake + Ninja)…"
cmake -S . -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DATIVASTAGE_BUILD_TESTS=ON \
  -DATIVASTAGE_BUILD_APP=ON \
  -DCMAKE_PREFIX_PATH="$QT_PREFIX" \
  || { echo "!! Erro ao configurar."; read -r -p "Enter…" _; exit 1; }

echo "→ Compilando…"
cmake --build "$BUILD_DIR" || { echo "!! Erro ao compilar."; read -r -p "Enter…" _; exit 1; }

echo "→ Rodando testes…"
ctest --test-dir "$BUILD_DIR" --output-on-failure || {
  echo "!! Testes falharam."; read -r -p "Enter…" _; exit 1; }

# 4) Montar o .app — só quando o executável GUI existir (a partir da Fase 1).
VERSION="$(tr -d '[:space:]' < VERSION 2>/dev/null || echo 0.0.0)"
BIN="$BUILD_DIR/bin/$APPNAME"
if [ ! -x "$BIN" ]; then
  echo ""
  echo "✅ Fundação compilada e testes verdes ($VERSION)."
  echo "   O aplicativo com janela chega na Fase 1 — a partir daí este mesmo"
  echo "   script monta e instala o $APPNAME.app automaticamente."
  read -r -p "Enter para fechar…" _; exit 0
fi

APP="/Applications/$APPNAME.app"
EXISTED="não"; [ -d "$APP" ] && EXISTED="sim"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp "$BIN" "$APP/Contents/MacOS/$APPNAME"; chmod +x "$APP/Contents/MacOS/$APPNAME"

# Ícone (opcional): gera .icns a partir de icon.png se existir.
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

# Empacota as bibliotecas do Qt dentro do .app (linkagem dinâmica LGPL).
if [ -n "$QT_PREFIX" ] && [ -x "$QT_PREFIX/bin/macdeployqt" ]; then
  echo "→ Empacotando bibliotecas Qt (macdeployqt)…"
  QMLDIR_FLAG=""; [ -d src/app/qml ] && QMLDIR_FLAG="-qmldir=src/app/qml"
  "$QT_PREFIX/bin/macdeployqt" "$APP" $QMLDIR_FLAG >/dev/null 2>&1 || \
    echo "  (aviso: macdeployqt retornou erro; o app pode exigir o Qt do Homebrew)"
fi

xattr -dr com.apple.quarantine "$APP" 2>/dev/null || true
[ "$EXISTED" = sim ] && echo "✅ $APPNAME atualizado ($VERSION)!" || echo "✅ $APPNAME $VERSION instalado!"
open "$APP"
