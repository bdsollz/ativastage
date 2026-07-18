#!/bin/bash
# git-auto — sincroniza tudo com o GitHub em UM duplo-clique, sem digitar nada.
# Limpa lock obsoleto, adiciona, commita (mensagem automática) e faz push da
# branch atual. Mensagem custom opcional: git-auto.command "minha mensagem".
set -o pipefail
cd "$(dirname "$0")/.." || exit 1   # raiz do projeto

# Remove trava obsoleta que o ambiente do agente pode ter deixado no .git.
rm -f .git/index.lock 2>/dev/null

BRANCH="$(git branch --show-current 2>/dev/null)"
if [ -z "$BRANCH" ]; then
  echo "!! Sem repositório Git aqui. Rode scripts/git-setup.command primeiro."
  exit 1
fi

echo "== AtivaStage — sincronizar (auto) =="
echo "   Branch: $BRANCH"

git add -A
if git diff --cached --quiet; then
  echo "-> Nada novo para commitar."
else
  MSG="${1:-chore: sync ($(date '+%Y-%m-%d %H:%M'))}"
  git commit -q -m "$MSG"
  echo "OK commit: $MSG"
fi

echo "-> Enviando para o GitHub..."
if git push origin "$BRANCH"; then
  echo "OK push concluido. CI: https://github.com/bdsollz/ativastage/actions"
else
  echo "!! push falhou (veja a mensagem acima)."
fi
echo ""
echo "Pode fechar esta janela."
