#!/bin/sh
# MosaicOS 0.50 Aero — prepara un host Debian para compilar la ISO i386.
# Uso: sudo ./prepare-build-host.sh
set -eu

[ "$(id -u)" -eq 0 ] || { echo "Uso: sudo $0" >&2; exit 1; }
[ -r /etc/os-release ] || { echo "No se detecta el sistema anfitrión." >&2; exit 1; }
. /etc/os-release

case "${ID:-}" in
  debian) ;;
  *)
    cat >&2 <<'EOF'
Este script solo admite Debian 12 (bookworm) o Debian 13 (trixie).

En Windows, macOS, Ubuntu u otras distribuciones usa la ruta Docker:
  docker build -t mosaicos-builder -f desktop/docker/Dockerfile desktop
  docker run --rm -v "$PWD:/work" mosaicos-builder

O usa la compilación en GitHub Actions (.github/workflows/build-iso.yml).
EOF
    exit 1
    ;;
esac

case "${VERSION_CODENAME:-}" in
  bookworm|trixie) ;;
  *) echo "Anfitrión admitido: Debian 12 (bookworm) o Debian 13 (trixie)." >&2; exit 1 ;;
esac

HOST_ARCH=$(dpkg --print-architecture)
case "$HOST_ARCH" in
  amd64|i386) ;;
  *) echo "El host debe ser amd64 o i386; detectado: $HOST_ARCH." >&2; exit 1 ;;
esac

echo "== MosaicOS: preparando host Debian $VERSION_CODENAME / $HOST_ARCH =="

apt-get update

# Herramientas comunes de live-build y análisis de la ISO.
BASE_PKGS="live-build debootstrap squashfs-tools xorriso mtools dosfstools \
  isolinux syslinux-common syslinux-utils file \
  ca-certificates curl git python3 shellcheck"

# Emulación i386 cuando el host es amd64.
EMU_PKGS=""
if [ "$HOST_ARCH" = amd64 ]; then
  EMU_PKGS="qemu-user-static binfmt-support qemu-system-x86 qemu-utils"
fi

# shellcheck disable=SC2086
DEBIAN_FRONTEND=noninteractive apt-get install -y $BASE_PKGS $EMU_PKGS

if [ "$HOST_ARCH" = amd64 ] && command -v update-binfmts >/dev/null 2>&1; then
  update-binfmts --enable qemu-i386 >/dev/null 2>&1 || true
fi

FREE_GB=$(df -Pk . | awk 'NR==2 {printf "%d", $4/1024/1024}')
echo
echo "Host preparado. Espacio libre en el directorio actual: ${FREE_GB} GiB."
echo "Se recomiendan al menos 30 GiB para compilar la ISO i386."
echo
echo "Compila con:"
echo "  sudo sh ./build.sh i386"
echo
echo "Después verifica y prueba:"
echo "  sh ./verify-iso.sh dist/MosaicOS-0.50-i386.iso"
echo "  qemu-system-i386 -m 2048 -cdrom dist/MosaicOS-0.50-i386.iso -boot d"
