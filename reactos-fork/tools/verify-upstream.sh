#!/bin/sh
# Comprueba que la fuente usada sea un checkout Git válido de ReactOS.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SRC=${1:-"$ROOT/upstream/reactos"}
if ! command -v git >/dev/null 2>&1; then
  echo "Error: git no está instalado." >&2; exit 1
fi
if [ ! -d "$SRC/.git" ]; then
  echo "Error: no es un checkout Git: $SRC" >&2; exit 1
fi
REMOTE=$(git -C "$SRC" remote get-url origin 2>/dev/null || true)
case "$REMOTE" in
  https://github.com/reactos/reactos.git|https://github.com/reactos/reactos|git@github.com:reactos/reactos.git) ;;
  *) echo "Aviso: origin no coincide con el repositorio oficial: ${REMOTE:-(sin origin)}" >&2 ;;
esac
COMMIT=$(git -C "$SRC" rev-parse HEAD)
BRANCH=$(git -C "$SRC" branch --show-current)
if ! git -C "$SRC" diff --quiet || ! git -C "$SRC" diff --cached --quiet; then
  echo "Error: hay cambios locales sin commit. Guarda o revierte los cambios antes del build." >&2
  exit 1
fi
printf 'Fuente: %s\nRama: %s\nCommit: %s\n' "$SRC" "${BRANCH:-detached HEAD}" "$COMMIT"
if [ -f "$ROOT/UPSTREAM_COMMIT.txt" ]; then
  PIN=$(sed -n '1p' "$ROOT/UPSTREAM_COMMIT.txt")
  if [ -n "$PIN" ] && [ "$PIN" != "$COMMIT" ]; then
    echo "Aviso: el commit no coincide con UPSTREAM_COMMIT.txt ($PIN)." >&2
  fi
fi
