#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sh -n "$ROOT/compatibility/tools/mosaic-compat"
"$ROOT/compatibility/tools/mosaic-compat" help >/dev/null
"$ROOT/compatibility/tools/mosaic-compat" status >/dev/null
printf 'MosaicOS 0.45 validation: OK\n'
