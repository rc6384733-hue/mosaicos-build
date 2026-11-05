#!/bin/sh
# Configura y compila ReactOS; no aplica parches ni altera la fuente upstream.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SRC=${1:-"$ROOT/upstream/reactos"}
BUILD=${2:-"$ROOT/build"}
"$ROOT/tools/verify-upstream.sh" "$SRC"
for tool in cmake ninja; do
  command -v "$tool" >/dev/null 2>&1 || { echo "Falta herramienta requerida: $tool" >&2; exit 1; }
done
[ -f "$SRC/configure.sh" ] || { echo "No se encontró configure.sh en: $SRC" >&2; exit 1; }
mkdir -p "$BUILD"
cd "$BUILD"
if [ ! -f build.ninja ]; then
  "$SRC/configure.sh"
else
  echo "Se reutiliza la configuración existente en $BUILD"
fi
ninja bootcd
ISO=$(find "$BUILD" -type f -name 'bootcd.iso' -print -quit)
if [ -z "$ISO" ]; then
  echo "La compilación terminó, pero no se encontró bootcd.iso dentro de $BUILD." >&2
  exit 1
fi
if command -v sha256sum >/dev/null 2>&1; then sha256sum "$ISO"; fi
printf '\nISO generada: %s\n' "$ISO"
printf 'No la instales en hardware real: primero valida el arranque en una VM.\n'
