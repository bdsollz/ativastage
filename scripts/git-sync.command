#!/bin/bash
# git-sync — adiciona, commita e envia as mudanças atuais (rodar na sua máquina).
# Dois cliques: útil para iterar (ex.: correções do CI).
set -o pipefail
cd "$(dirname "$0")/.." || exit 1   # raiz do projeto

BRANCH="$(git branch --show-current 2>/dev/null)"
[ -z "$BRANCH" ] && { echo "!! Sem repositório Git. Rode scripts/git-setup.command"; read -r -p "Enter…" _; exit 1; }

echo "== AtivaStage — sincronizar com o GitHub =="
echo "   Branch: $BRANCH"

git add -A
if git diff --cached --quiet; then
  echo "→ Nada novo para commitar."
else
  DEFAULT="chore: atualização ($(date +%Y-%m-%d\ %H:%M))"
  read -r -p "Mensagem do commit [$DEFAULT]: " MSG
  [ -z "$MSG" ] && MSG="$DEFAULT"
  git commit -q -m "$MSG"
  echo "✅ Commit criado."
fi

echo "→ Enviando…"
if git push origin "$BRANCH"; then
  echo "✅ Push concluído. CI: https://github.com/bdsollz/ativastage/actions"
else
  echo "!! Push falhou (ver mensagem acima)."
fi
echo ""
read -r -p "Enter para fechar…" _
