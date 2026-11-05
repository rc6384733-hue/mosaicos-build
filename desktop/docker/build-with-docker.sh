#!/bin/sh
# Compila la ISO i386 usando Docker. Ejecuta desde cualquier ubicación.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
cd "$ROOT"

if ! command -v docker >/dev/null 2>&1; then
  echo "Docker no está instalado o no está en PATH." >&2
  exit 1
fi

echo "== Construyendo la imagen mosaicos-builder =="
docker build -f desktop/docker/Dockerfile -t mosaicos-builder desktop

echo
echo "== Compilando MosaicOS 0.50 i386 =="
docker run --rm -v "$ROOT:/work" mosaicos-builder i386

echo
echo "ISO esperada en: desktop/dist/MosaicOS-0.50-i386.iso"
