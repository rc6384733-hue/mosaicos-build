#!/bin/sh
set -eu
ISO=${1:-}
[ -n "$ISO" ] || { echo "Uso: $0 imagen.iso" >&2; exit 2; }
[ -s "$ISO" ] || { echo "ISO inexistente o vacía: $ISO" >&2; exit 1; }
command -v file >/dev/null 2>&1 || { echo 'Instala file.' >&2; exit 1; }
command -v xorriso >/dev/null 2>&1 || { echo 'Instala xorriso.' >&2; exit 1; }
echo 'Identificación:'; file "$ISO"
echo '\nVolumen ISO:'; xorriso -indev "$ISO" -pvd_info 2>&1 | grep -E 'Volume Id|Volume Id:' || true
echo '\nArchivos de arranque:'; xorriso -indev "$ISO" -find / -name 'vmlinuz*' -o -name 'initrd*' 2>&1 | tail -20
sha256sum "$ISO"
echo '\nVerificación estructural terminada. Esto no sustituye una prueba real de arranque en QEMU.'
