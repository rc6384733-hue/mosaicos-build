#!/bin/sh
# Arranque de prueba de la ISO en QEMU; usa únicamente un disco virtual descartable.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ISO=${1:-"$ROOT/build/boot/bootcd.iso"}
RAM=${MOSAICOS_QEMU_RAM:-512}
QEMU=${QEMU:-qemu-system-i386}
[ -f "$ISO" ] || { echo "No se encontró la ISO: $ISO" >&2; exit 1; }
command -v "$QEMU" >/dev/null 2>&1 || { echo "No se encontró $QEMU. Instala QEMU para probar la ISO." >&2; exit 1; }
case "$RAM" in *[!0-9]*|'') echo "MOSAICOS_QEMU_RAM debe ser un número en MB." >&2; exit 1;; esac
[ "$RAM" -ge 256 ] || { echo "Usa al menos 256 MB para esta prueba." >&2; exit 1; }
exec "$QEMU" -m "$RAM" -boot d -cdrom "$ISO" -nic user,model=rtl8139 -vga std
