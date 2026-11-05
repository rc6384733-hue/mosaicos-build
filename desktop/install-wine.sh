#!/bin/sh
set -eu

if [ "$(id -u)" -ne 0 ]; then
  echo "Ejecuta con sudo: sudo ./install-wine.sh" >&2
  exit 1
fi
if ! command -v apt-get >/dev/null 2>&1 || ! grep -qi '^ID=debian' /etc/os-release; then
  echo "Este instalador está pensado para MosaicOS Desktop basado en Debian." >&2
  exit 1
fi

dpkg --add-architecture i386
apt-get update
apt-get install wine wine32:i386
printf '\nWine quedó instalado si apt terminó sin errores. La compatibilidad debe comprobarse por aplicación.\n'
