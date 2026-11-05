#!/bin/sh
# MosaicOS 0.50 Aero — construye una ISO live/instalable de 32 bits (i386).
# Requiere un host Debian bookworm/trixie (amd64 o i386), root, red y ~30 GiB libres.
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
fail() { echo "ERROR: $*" >&2; exit 1; }

ARCH=${1:-i386}
[ "$ARCH" = i386 ] || fail "MosaicOS 0.50 es una edición de 32 bits. Usa: sudo ./build.sh i386"

CODENAME=bookworm
PROFILE=mosaic-desktop-i386.packages
VOLUME=MOSAICOS_050_I386
VERSION=$(cat "$SCRIPT_DIR/../VERSION" 2>/dev/null || echo 0.50)
DIST="$SCRIPT_DIR/dist"
WORK_DIR="$SCRIPT_DIR/build/i386"
LOG="$WORK_DIR/build.log"

[ "$(id -u)" -eq 0 ] || fail "Ejecuta como root: sudo ./build.sh i386"
[ -r /etc/os-release ] || fail "No se encuentra /etc/os-release."
. /etc/os-release
case "${ID:-}" in
  debian) ;;
  *) fail "Se requiere un host Debian (bookworm o trixie). Host detectado: ${ID:-desconocido}." ;;
esac
[ "$(dpkg --print-architecture 2>/dev/null || true)" = amd64 ] || [ "$(dpkg --print-architecture 2>/dev/null || true)" = i386 ] \
  || fail "El host debe ser amd64 o i386."

for tool in lb debootstrap xorriso mksquashfs sha256sum awk df tee; do
  command -v "$tool" >/dev/null 2>&1 || fail "Falta '$tool'. Ejecuta ./prepare-build-host.sh"
done

# La ISO i386 se puede construir nativamente en i386 o con emulación qemu en amd64.
if [ "$(dpkg --print-architecture)" = amd64 ]; then
  if ! command -v qemu-debootstrap >/dev/null 2>&1 && ! command -v qemu-i386-static >/dev/null 2>&1; then
    fail "Para construir i386 desde amd64 instala qemu-user-static y binfmt-support (./prepare-build-host.sh)."
  fi
fi

FREE_KB=$(df -Pk "$SCRIPT_DIR" | awk 'NR==2 {print $4}')
[ -n "$FREE_KB" ] || fail "No se pudo comprobar el espacio libre."
MIN_FREE_GIB=${MOSAIC_MIN_FREE_GIB:-20}
[ "$FREE_KB" -ge $((MIN_FREE_GIB * 1024 * 1024)) ] || \
  fail "Se requieren ${MIN_FREE_GIB} GiB libres; disponibles: $((FREE_KB / 1024 / 1024)) GiB. Puedes ajustar MOSAIC_MIN_FREE_GIB."

CONFIG_DIR="$SCRIPT_DIR/config"
[ -f "$CONFIG_DIR/package-lists/$PROFILE" ] || fail "No existe el perfil $PROFILE."

mkdir -p "$WORK_DIR" "$DIST"
rm -rf "$WORK_DIR/config"
mkdir -p "$WORK_DIR/config"
cp -R "$CONFIG_DIR/." "$WORK_DIR/config/"

# Los ZIP extraídos en Windows/Linux pueden perder el bit de ejecución.
chmod 0755 "$WORK_DIR"/config/hooks/live/*.hook.chroot 2>/dev/null || true

# live-build no debe mezclar listas de paquetes de otras arquitecturas.
find "$WORK_DIR/config/package-lists" -maxdepth 1 -type f -name '*.packages' -delete
cp "$CONFIG_DIR/package-lists/$PROFILE" "$WORK_DIR/config/package-lists/mosaic-desktop.list.chroot"

cd "$WORK_DIR"
lb clean --purge >/dev/null 2>&1 || true

{
  echo "MosaicOS $VERSION — build $(date -Is 2>/dev/null || date)"
  echo "Host: $(uname -srm) / $(dpkg --print-architecture)"
  echo "Objetivo: $ARCH / Debian $CODENAME"
} | tee "$LOG"

# Opciones de live-build:
# - i386 + kernel 686-pae para equipos de 32 bits con más de 4 GiB (PAE).
# - arranque BIOS con syslinux/isolinux; instalación mediante el instalador Debian live.
# - LXQt/Openbox + Wine32 + apps nativas i386.
lb config \
  --distribution "$CODENAME" \
  --architectures i386 \
  --binary-images iso-hybrid \
  --archive-areas "main contrib non-free non-free-firmware" \
  --iso-application "MosaicOS $VERSION Aero i386" \
  --iso-volume "$VOLUME" \
  --iso-publisher "MosaicOS Project" \
  --bootappend-live "boot=live components hostname=mosaicos username=mosaic locales=es_ES.UTF-8 keyboard-layouts=es timezone=Europe/Madrid console=tty0 console=ttyS0,115200 quiet splash" \
  --bootloaders syslinux \
  --linux-flavours 686-pae \
  --debian-installer live \
  --debian-installer-distribution "$CODENAME" \
  --debian-installer-gui true \
  --apt-recommends true \
  --memtest none \
  --checksums sha256 \
  --verbose

lb build 2>&1 | tee -a "$LOG"

ISO=$(find "$WORK_DIR" -maxdepth 1 -type f -name '*.iso' -print -quit)
[ -n "$ISO" ] && [ -s "$ISO" ] || fail "No se encontró una ISO válida tras live-build. Revisa $LOG."
DEST="$DIST/MosaicOS-$VERSION-i386.iso"
cp "$ISO" "$DEST"
( cd "$DIST" && sha256sum "MosaicOS-$VERSION-i386.iso" > "MosaicOS-$VERSION-i386.iso.sha256" )

printf '\nISO creada: %s\n' "$DEST"
printf 'Tamaño: %s\n' "$(du -h "$DEST" | awk '{print $1}')"
printf 'SHA256: %s\n' "$(cut -d' ' -f1 "$DEST.sha256")"
printf '\nSiguiente paso: sh ./verify-iso.sh "%s" y prueba en QEMU o hardware real.\n' "$DEST"
printf 'La compilación no demuestra el arranque. No instales sobre un disco con datos sin copia.\n'
