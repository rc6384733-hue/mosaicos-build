#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DEST=${1:-"$ROOT/upstream/reactos"}
if ! command -v git >/dev/null 2>&1; then
  echo "Error: git no está instalado." >&2; exit 1
fi
if [ -e "$DEST" ]; then
  echo "Error: el destino ya existe: $DEST" >&2
  echo "No se sobrescribe un árbol existente. Elige otra ruta o muévelo manualmente." >&2
  exit 1
fi
mkdir -p "$(dirname -- "$DEST")"
git clone https://github.com/reactos/reactos.git "$DEST"
COMMIT=$(git -C "$DEST" rev-parse HEAD)
printf '%s\n' "$COMMIT" > "$ROOT/UPSTREAM_COMMIT.txt"
git -C "$DEST" switch -c mosaicos/main
echo "ReactOS clonado en: $DEST"
echo "Commit upstream: $COMMIT"
echo "Rama creada: mosaicos/main"
