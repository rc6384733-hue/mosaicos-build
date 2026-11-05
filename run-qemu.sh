#!/bin/bash
# MosaicOS 0.18 — Arranque rápido en QEMU
# Sistema operativo original e independiente (no es Linux ni Windows)

set -e

cd "$(dirname "$0")"

echo "=== MosaicOS 0.18 — Compilando si es necesario ==="
make all

echo ""
echo "=== Arrancando en QEMU (256 MiB) ==="
echo "Controles: flechas + Enter, F1-F5, Terminal → help"
echo "Para salir de QEMU: Ctrl+Alt+G y luego cerrar la ventana, o Ctrl+A X"
echo ""

exec qemu-system-i386 -kernel mosaicos.bin -m 256 -netdev user,id=n0 -device rtl8139,netdev=n0 -device piix3-usb-uhci -device usb-tablet -name "MosaicOS 0.18"
