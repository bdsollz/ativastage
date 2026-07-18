#!/bin/bash
# git-setup — inicializa o repositório Git do AtivaStage (rodar na sua máquina).
# Dois cliques: cria o repo, a branch da fase e o commit inicial.
# (O ambiente assistido por IA não consegue rodar git nesta pasta por causa de
#  restrições de exclusão do mount; por isso este passo roda localmente.)
set -o pipefail
cd "$(dirname "$0")/.." || exit 1   # raiz do projeto (pasta acima de scripts/)

BRANCH="phase/0-foundation"
NAME="Bruno Oliver"
EMAIL="timeweverson@gmail.com"

echo "== AtivaStage — configuração do Git =="
echo "   Pasta: $(pwd)"

# Se existir um .git quebrado/parcial, oferecer recomeçar limpo.
if [ -d .git ]; then
  if ! git rev-parse HEAD >/dev/null 2>&1; then
    echo "→ Encontrei um .git sem commits (provavelmente incompleto)."
    read -r -p "  Recomeçar do zero (apaga só o .git, não seu código)? [s/N] " R
    case "$R" in s|S|y|Y) rm -rf .git; echo "  .git removido." ;; *) echo "  Mantendo o .git atual." ;; esac
  else
    echo "→ Repositório já inicializado. Vou apenas adicionar e commitar o que falta."
  fi
fi

# Init + identidade (local, não mexe na sua config global).
if [ ! -d .git ]; then
  git init -q
  git symbolic-ref HEAD refs/heads/main
fi
git config user.name  >/dev/null 2>&1 || git config user.name  "$NAME"
git config user.email >/dev/null 2>&1 || git config user.email "$EMAIL"

# Branch da fase.
git rev-parse --verify "$BRANCH" >/dev/null 2>&1 && git checkout -q "$BRANCH" || git checkout -q -b "$BRANCH"

# Commit.
git add -A
if git diff --cached --quiet; then
  echo "→ Nada novo para commitar."
else
  git commit -q -m "Fase 0: fundação (repo, CMake+vcpkg, libpal, libpersistence, CI, auto-compilar, checkpoint de estado)"
  echo "✅ Commit criado."
fi

echo ""
echo "== Estado =="
git --no-pager log --oneline -1 2>/dev/null
git --no-pager branch
echo ""
echo "== Próximo passo: enviar ao GitHub =="
echo "  1) Crie um repositório PRIVADO vazio no GitHub (sem README)."
echo "  2) Rode (troque a URL):"
echo "       git remote add origin git@github.com:SEU_USUARIO/AtivaStage.git"
echo "       git push -u origin $BRANCH"
echo "     O GitHub Actions vai compilar e testar em macOS e Windows."
echo ""
read -r -p "Enter para fechar…" _
