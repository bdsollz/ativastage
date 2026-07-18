#!/bin/bash
# git-push — conecta ao GitHub e envia a branch atual (rodar na sua máquina).
# Dois cliques: configura o remote 'origin' e faz o push.
set -o pipefail
cd "$(dirname "$0")/.." || exit 1   # raiz do projeto

REPO_URL="https://github.com/bdsollz/ativastage.git"
BRANCH="$(git branch --show-current 2>/dev/null)"
[ -z "$BRANCH" ] && BRANCH="phase/0-foundation"

echo "== AtivaStage — push para o GitHub =="
echo "   Repo:   $REPO_URL"
echo "   Branch: $BRANCH"

if [ ! -d .git ]; then
  echo "!! Este diretório não tem repositório Git. Rode antes: scripts/git-setup.command"
  read -r -p "Enter para fechar…" _; exit 1
fi

# Configura/atualiza o remote 'origin'.
if git remote get-url origin >/dev/null 2>&1; then
  git remote set-url origin "$REPO_URL"
  echo "→ Remote 'origin' atualizado."
else
  git remote add origin "$REPO_URL"
  echo "→ Remote 'origin' adicionado."
fi

# Opção de limpar credencial antiga (causa comum de erro 403).
read -r -p "Limpar a credencial salva do github.com nas Chaves e pedir de novo? [s/N] " C
case "$C" in
  s|S|y|Y)
    printf "protocol=https\nhost=github.com\n\n" | git credential-osxkeychain erase 2>/dev/null
    echo "  Credencial do github.com removida das Chaves."
    ;;
esac

echo "→ Enviando… (o GitHub deve pedir usuário e um TOKEN como senha)"
echo "  Usuário: bdsollz"
echo "  Senha:   cole o Personal Access Token (classic, escopo 'repo', começa com ghp_)."
if git push -u origin "$BRANCH"; then
  echo ""
  echo "✅ Push concluído. Acompanhe o CI em:"
  echo "   https://github.com/bdsollz/ativastage/actions"
else
  echo ""
  echo "!! O push falhou. Causas comuns:"
  echo "   - Autenticação: crie/use um Personal Access Token (escopo 'repo')."
  echo "   - O repositório remoto não está vazio: ele deve ser criado SEM README."
fi
echo ""
read -r -p "Enter para fechar…" _
