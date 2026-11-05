#!/bin/bash
# MosaicOS 0.18 — Generar ISO arrancable con GRUB
# Sistema operativo original e independiente (no es Linux ni Windows)

set -e

cd "$(dirname "$0")"

echo "=== MosaicOS 0.18 — Compilando ==="
make all

echo ""
echo "=== Generando ISO Multiboot ==="
make iso

echo ""
echo "ISO generada: MosaicOS.iso"
echo ""
echo "Probar en QEMU:"
echo "  qemu-system-i386 -cdrom MosaicOS.iso -m 256"
echo ""
echo "Grabar en USB de prueba (¡solo USB sin datos importantes!):"
echo "  sudo dd if=MosaicOS.iso of=/dev/sdX bs=4M status=progress && sync"
echo ""
echo "Recuerda: MosaicOS es un prototipo experimental."
echo "Úsalo únicamente en entornos aislados."
