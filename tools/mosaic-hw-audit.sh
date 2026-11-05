#!/bin/sh
# Inventario de solo lectura para evaluar compatibilidad de MosaicOS.
set -eu
printf '%s\n' '=== MosaicOS Hardware Audit ==='
printf 'Fecha UTC: '; date -u '+%Y-%m-%dT%H:%M:%SZ' 2>/dev/null || true
printf 'Kernel: '; uname -srmo
printf 'Arquitectura usuario: '; uname -m
printf '\n-- CPU --\n'
if [ -r /proc/cpuinfo ]; then
  awk -F: '/^(model name|Hardware|vendor_id|cpu family|model|flags)[[:space:]]*:/ {gsub(/^[ \t]+/,"",$2); if ($1=="flags") {print "flags: " substr($2,1,240)} else print $1 ": " $2; if (++n>=12) exit}' /proc/cpuinfo
else echo 'No disponible'; fi
printf '\n-- RAM y swap --\n'
if [ -r /proc/meminfo ]; then grep -E '^(MemTotal|MemAvailable|SwapTotal|SwapFree):' /proc/meminfo; else echo 'No disponible'; fi
printf '\n-- Almacenamiento --\n'
if command -v lsblk >/dev/null 2>&1; then lsblk -o NAME,TYPE,SIZE,ROTA,TRAN,MODEL 2>/dev/null || true; else echo 'lsblk no instalado'; fi
printf '\n-- Gráficos --\n'
if command -v lspci >/dev/null 2>&1; then lspci 2>/dev/null | grep -Ei 'VGA|3D|Display' || true; else echo 'lspci no instalado'; fi
printf '\n-- Red --\n'
if command -v lspci >/dev/null 2>&1; then lspci 2>/dev/null | grep -Ei 'Ethernet|Network|Wireless' || true; fi
printf '\n-- Firmware / virtualización --\n'
[ -d /sys/firmware/efi ] && echo 'UEFI: sí' || echo 'UEFI: no detectado (posible BIOS/legacy o VM)'
grep -m1 -Eo '\b(vmx|svm)\b' /proc/cpuinfo 2>/dev/null | sed 's/^/Virtualización CPU: /' || echo 'Virtualización CPU: no detectada'
printf '\nAuditoría de solo lectura finalizada. No se modificó el sistema.\n'
