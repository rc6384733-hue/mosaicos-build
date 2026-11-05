#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
for file in \
  "$ROOT/branding/PRODUCT.txt" \
  "$ROOT/UPSTREAM.md" \
  "$ROOT/BUILD_STATUS.md" \
  "$ROOT/tools/clone-upstream.sh" \
  "$ROOT/tools/verify-upstream.sh" \
  "$ROOT/tools/build-reactos.sh" \
  "$ROOT/tools/run-qemu.sh"; do
  [ -s "$file" ] || { echo "Falta archivo requerido: $file" >&2; exit 1; }
done
for script in "$ROOT"/tools/*.sh; do
  [ -x "$script" ] || { echo "Script sin permiso ejecutable: $script" >&2; exit 1; }
  sh -n "$script"
done
echo "MosaicOS ReactOS toolkit: OK"
echo "Validación estática completada; no compila ReactOS ni genera una ISO."
