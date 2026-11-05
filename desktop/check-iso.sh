#!/bin/sh
set -eu
ISO=${1:-}
if [ -z "$ISO" ] || [ ! -s "$ISO" ]; then
  echo "Uso: ./check-iso.sh ruta/a/MosaicOS.iso" >&2
  exit 2
fi
command -v xorriso >/dev/null 2>&1 || { echo "Falta xorriso" >&2; exit 1; }
command -v sha256sum >/dev/null 2>&1 || { echo "Falta sha256sum" >&2; exit 1; }
echo "== Integridad =="
sha256sum "$ISO"
echo "== Contenido ISO =="
xorriso -indev "$ISO" -find / -maxdepth 3 -type f -print 2>/dev/null | head -80
echo "== Arranque QEMU (opcional) =="
if command -v qemu-system-i386 >/dev/null 2>&1; then
  echo "QEMU disponible. Ejecuta manualmente: qemu-system-i386 -m 2048 -cdrom '$ISO' -boot d"
else
  echo "QEMU no instalado; omitiendo prueba de arranque."
fi
