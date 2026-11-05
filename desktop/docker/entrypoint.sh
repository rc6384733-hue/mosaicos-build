#!/bin/sh
# Punto de entrada del contenedor de compilación de MosaicOS.
set -eu

if [ ! -d /work/desktop ]; then
  cat >&2 <<'EOF'
No se encontró /work/desktop.

Monta la raíz del proyecto en /work:
  docker run --rm -v "$PWD:/work" mosaicos-builder

En PowerShell:
  docker run --rm -v "${PWD}:/work" mosaicos-builder
EOF
  exit 1
fi

cd /work/desktop
# Los permisos de ejecución pueden perderse en volúmenes de Windows/macOS.
chmod +x build.sh prepare-build-host.sh verify-iso.sh 2>/dev/null || true

# En CI el contenedor se ejecuta con --privileged para poder registrar la
# emulación i386. Sin este registro, debootstrap no puede ejecutar dpkg i386
# dentro del chroot y la compilación falla en "Unpacking the base system".
if command -v update-binfmts >/dev/null 2>&1; then
  if ! grep -q binfmt_misc /proc/mounts 2>/dev/null; then
    mkdir -p /proc/sys/fs/binfmt_misc 2>/dev/null || true
    mount -t binfmt_misc binfmt_misc /proc/sys/fs/binfmt_misc 2>/dev/null || true
  fi
  update-binfmts --enable qemu-i386 >/dev/null 2>&1 || true
fi

exec sh ./build.sh "${1:-i386}"
