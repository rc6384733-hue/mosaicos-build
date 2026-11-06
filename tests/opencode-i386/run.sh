#!/bin/sh
# Prueba de CI: ejecuta el OpenCode oficial x86_64 con qemu-x86_64 de 32 bits.
#
# El trabajo de CI instala qemu-user:i386 en un contenedor amd64. El kernel
# ejecuta qemu-x86_64 como proceso i386 nativo (sin emulación anidada), que a
# su vez emula el binario x86_64 de OpenCode. Es el escenario más parecido a
# MosaicOS 0.50 i386 que se puede reproducir sin arrancar la ISO.
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
PREFIX=${OPENCODE_PREFIX:-/opt/opencode}
WRAPPER="$ROOT/desktop/config/includes.chroot/usr/local/bin/opencode"
INSTALLER="$ROOT/desktop/config/includes.chroot/usr/local/lib/mosaicos/opencode-install.sh"
QEMU=${OPENCODE_QEMU:-/usr/bin/qemu-x86_64}
LOG=${OPENCODE_TEST_LOG:-/tmp/opencode-i386.log}

fail() {
  echo "FALLO: $*" >&2
  if [ -f "$LOG" ]; then
    echo "== Salida de la última prueba ==" >&2
    tail -60 "$LOG" >&2 || true
  fi
  exit 1
}

echo "== Arquitectura del sistema =="
uname -m
dpkg --print-architecture 2>/dev/null || true

echo "== Emulador =="
[ -x "$QEMU" ] || fail "No existe $QEMU"
file "$QEMU"
file "$QEMU" | grep -q 'ELF 32-bit' || \
  fail "qemu-x86_64 no es un ejecutable de 32 bits; la prueba no sería representativa"
"$QEMU" --version | head -n 1

echo "== Instalación de OpenCode =="
sh "$INSTALLER"
install -m 0755 "$WRAPPER" /usr/local/bin/opencode
[ -x /usr/local/bin/opencode ] || fail "No se instaló el lanzador"

echo "== Comprobación de archivos =="
for path in \
  "$PREFIX/opencode" \
  "$PREFIX/x86_64/lib/ld-musl-x86_64.so.1" \
  "$PREFIX/x86_64/lib/libc.musl-x86_64.so.1" \
  "$PREFIX/x86_64/usr/lib/libstdc++.so.6" \
  "$PREFIX/x86_64/usr/lib/libgcc_s.so.1"; do
  [ -e "$path" ] || fail "Falta $path"
done

run_and_log() {
  # run_and_log ETIQUETA COMANDO...
  label=$1
  shift
  echo "== $label =="
  : > "$LOG"
  set +e
  timeout 1800 "$@" > "$LOG" 2>&1
  rc=$?
  set -e
  cat "$LOG"
  if [ "$rc" -ne 0 ]; then
    fail "$label terminó con código $rc"
  fi
}

run_and_log "opencode --version" /usr/local/bin/opencode --version
grep -q '1\.19\.5' "$LOG" || fail "La versión no es 1.19.5"

run_and_log "opencode --help" /usr/local/bin/opencode --help
grep -qi 'opencode' "$LOG" || fail "La ayuda no menciona OpenCode"

echo "OK: OpenCode 1.19.5 arranca con qemu-x86_64 de 32 bits sobre i386."
