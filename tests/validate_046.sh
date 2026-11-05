#!/bin/sh
# Compatibilidad: la validación vigente desde 0.47 reemplaza las aserciones históricas 0.46.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
"$ROOT/tests/validate_047.sh"
