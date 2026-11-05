#!/bin/sh
# Punto de entrada del contenedor de compilación de MosaicOS.
set -eu

if [ ! -d /work/desktop ]; then
  cat >&2 <<'EOF'
No se encontró /work/desktop.

Monta la raíz del proyecto en /work:
  docker run --rm -v "$PWD:/work" mosaicos-builder

En PowerShell:
  docker run --rm -v "${PWD}:/work" mosaicos-builder
EOF
  exit 1
fi

cd /work/desktop
# Los permisos de ejecución pueden perderse en volúmenes de Windows/macOS.
chmod +x build.sh prepare-build-host.sh verify-iso.sh 2>/dev/null || true

exec sh ./build.sh "${1:-i386}"
