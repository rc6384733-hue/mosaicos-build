#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
for script in "$ROOT/desktop/build.sh" "$ROOT/desktop/prepare-build-host.sh" "$ROOT/desktop/verify-iso.sh" "$ROOT/desktop/check-iso.sh"; do
  sh -n "$script"
done
python3 "$ROOT/tests/validate_047.py"
