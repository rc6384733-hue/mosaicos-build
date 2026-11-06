#!/bin/sh
# MosaicOS — instala OpenCode 1.19.5 para ejecutarlo sobre i386 mediante qemu-user.
#
# OpenCode no publica un binario Linux de 32 bits. La alternativa compatible es
# usar el binario oficial x86_64 con libc musl y ejecutarlo con qemu-x86_64
# (qemu-user i386), que es un proceso de 32 bits.
#
# Este instalador descarga los artefactos oficiales, comprueba su SHA-256 y
# monta /opt/opencode con:
#   opencode                         binario x86_64 (Bun, musl)
#   x86_64/lib/ld-musl-x86_64.so.1   cargador de musl x86_64
#   x86_64/lib/libc.musl-x86_64.so.1 libc x86_64
#   x86_64/usr/lib/libstdc++.so.6    runtime C++ x86_64
#   x86_64/usr/lib/libgcc_s.so.1     runtime GCC x86_64
#
# Se puede ejecutar durante la compilación de la ISO y en la prueba de CI.
set -eu

PREFIX=${OPENCODE_PREFIX:-/opt/opencode}
WORKDIR=${OPENCODE_WORKDIR:-${TMPDIR:-/tmp}/mosaicos-opencode.$$}
KEEP_DOWNLOADS=${OPENCODE_KEEP_DOWNLOADS:-0}

OPENCODE_VERSION=1.19.5
OPENCODE_TGZ="opencode-linux-x64-baseline-musl-${OPENCODE_VERSION}.tgz"
OPENCODE_URL="https://registry.npmjs.org/opencode-linux-x64-baseline-musl/-/${OPENCODE_TGZ}"
OPENCODE_TGZ_SHA256=1f4c1fcc646d4d03d1ea0997a54d1e8b9fe17af6356ac54dde2c65e53fedc682
OPENCODE_BIN_SHA256=528058cf8f2a7196ec94dc113052b785f8f94f6802ced3980ad7359369ddacc8

ALPINE_VERSION=3.20
ALPINE_ARCH=x86_64
ALPINE_BASE="https://dl-cdn.alpinelinux.org/alpine/v${ALPINE_VERSION}/main/${ALPINE_ARCH}"

MUSL_APK=musl-1.2.5-r4.apk
MUSL_SHA256=b255cdcf4f64eb301f4dbd40d1fbdc0fcb021613b3f0d124a41eeff62e2bc7d8
LIBSTDCXX_APK=libstdc++-13.2.1_git20240309-r1.apk
LIBSTDCXX_SHA256=af0fe894ef5051116e321bf4753a10fffa85abc2b71f30b2e949467775421ace
LIBGCC_APK=libgcc-13.2.1_git20240309-r1.apk
LIBGCC_SHA256=f348d99e10b5267566afe6f80861661b08cb5aa43a6d4d1c8f1792b5001d0995

log() { printf '[mosaic-opencode] %s\n' "$*"; }
fail() { printf '[mosaic-opencode] ERROR: %s\n' "$*" >&2; exit 1; }

sha256_of() {
  sha256sum "$1" | awk '{print $1}'
}

fetch() {
  # fetch URL DESTINO SHA256_ESPERADO
  url=$1
  dest=$2
  expected=$3
  if [ -f "$dest" ] && [ "$(sha256_of "$dest")" = "$expected" ]; then
    log "Reutilizando $(basename "$dest") (SHA-256 correcto)"
    return 0
  fi
  log "Descargando $(basename "$dest")"
  attempt=1
  while [ "$attempt" -le 4 ]; do
    if curl -fL --retry 3 --retry-delay 5 --connect-timeout 30 \
        --no-progress-meter -o "$dest" "$url"; then
      break
    fi
    log "Descarga fallida (intento $attempt/4): $url"
    attempt=$((attempt + 1))
    [ "$attempt" -le 4 ] || fail "No se pudo descargar $url"
    sleep 5
  done
  actual=$(sha256_of "$dest")
  [ "$actual" = "$expected" ] || \
    fail "SHA-256 incorrecto para $(basename "$dest"): $actual != $expected"
}

cleanup() {
  if [ "$KEEP_DOWNLOADS" != 1 ] && [ -d "$WORKDIR" ]; then
    rm -rf "$WORKDIR"
  fi
}
trap cleanup EXIT INT TERM

[ "$(id -u)" = 0 ] || fail "Este instalador debe ejecutarse como root."
for tool in curl tar sha256sum awk install; do
  command -v "$tool" >/dev/null 2>&1 || fail "Falta la herramienta '$tool'."
done

mkdir -p "$WORKDIR/opencode" "$WORKDIR/musl" "$WORKDIR/libstdcxx" "$WORKDIR/libgcc"
mkdir -p "$PREFIX/x86_64/lib" "$PREFIX/x86_64/usr/lib"

fetch "$OPENCODE_URL" "$WORKDIR/$OPENCODE_TGZ" "$OPENCODE_TGZ_SHA256"
fetch "$ALPINE_BASE/$MUSL_APK" "$WORKDIR/$MUSL_APK" "$MUSL_SHA256"
fetch "$ALPINE_BASE/$LIBSTDCXX_APK" "$WORKDIR/$LIBSTDCXX_APK" "$LIBSTDCXX_SHA256"
fetch "$ALPINE_BASE/$LIBGCC_APK" "$WORKDIR/$LIBGCC_APK" "$LIBGCC_SHA256"

log "Extrayendo el binario de OpenCode"
tar -xzf "$WORKDIR/$OPENCODE_TGZ" -C "$WORKDIR/opencode" package/bin/opencode
[ -f "$WORKDIR/opencode/package/bin/opencode" ] || \
  fail "El paquete de OpenCode no contiene package/bin/opencode."
actual_bin=$(sha256_of "$WORKDIR/opencode/package/bin/opencode")
[ "$actual_bin" = "$OPENCODE_BIN_SHA256" ] || \
  fail "SHA-256 del binario de OpenCode inesperado: $actual_bin"

log "Extrayendo bibliotecas x86_64 de Alpine ${ALPINE_VERSION}"
# Se extraen solo los archivos regulares necesarios: los APK también contienen
# enlaces simbólicos que no hacen falta y que pueden fallar en algunos entornos.
tar -xzf "$WORKDIR/$MUSL_APK" -C "$WORKDIR/musl" lib/ld-musl-x86_64.so.1
tar -xzf "$WORKDIR/$LIBSTDCXX_APK" -C "$WORKDIR/libstdcxx" usr/lib/libstdc++.so.6.0.32
tar -xzf "$WORKDIR/$LIBGCC_APK" -C "$WORKDIR/libgcc" usr/lib/libgcc_s.so.1

[ -f "$WORKDIR/musl/lib/ld-musl-x86_64.so.1" ] || fail "musl no contiene ld-musl-x86_64.so.1"
libstdcpp="$WORKDIR/libstdcxx/usr/lib/libstdc++.so.6.0.32"
[ -f "$libstdcpp" ] || fail "libstdc++ no contiene libstdc++.so.6.0.32"
[ -f "$WORKDIR/libgcc/usr/lib/libgcc_s.so.1" ] || fail "libgcc no contiene libgcc_s.so.1"

install -m 0755 "$WORKDIR/opencode/package/bin/opencode" "$PREFIX/opencode"
install -m 0755 "$WORKDIR/musl/lib/ld-musl-x86_64.so.1" "$PREFIX/x86_64/lib/ld-musl-x86_64.so.1"
ln -sfn ld-musl-x86_64.so.1 "$PREFIX/x86_64/lib/libc.musl-x86_64.so.1"
libstdcpp_name=$(basename "$libstdcpp")
install -m 0755 "$libstdcpp" "$PREFIX/x86_64/usr/lib/$libstdcpp_name"
ln -sfn "$libstdcpp_name" "$PREFIX/x86_64/usr/lib/libstdc++.so.6"
install -m 0755 "$WORKDIR/libgcc/usr/lib/libgcc_s.so.1" "$PREFIX/x86_64/usr/lib/libgcc_s.so.1"

# musl consulta este archivo al resolver DT_NEEDED. También se pasa
# LD_LIBRARY_PATH desde el lanzador para no depender de la ruta global.
mkdir -p /etc
cat > /etc/ld-musl-x86_64.path <<EOF
$PREFIX/x86_64/lib
$PREFIX/x86_64/usr/lib
EOF
chmod 0644 /etc/ld-musl-x86_64.path

# El intérprete PT_INTERP del binario es /lib/ld-musl-x86_64.so.1. El enlace
# permite que qemu reejecute binarios x86_64 hijos y que el propio binario
# encuentre su cargador si se invoca de forma nativa en un sistema amd64.
mkdir -p /lib
ln -sfn "$PREFIX/x86_64/lib/ld-musl-x86_64.so.1" /lib/ld-musl-x86_64.so.1

cat > "$PREFIX/MANIFEST" <<EOF
OpenCode ${OPENCODE_VERSION} (linux-x64-baseline-musl)
Binario: ${OPENCODE_BIN_SHA256}
Tarball: ${OPENCODE_TGZ_SHA256}
musl ${MUSL_APK}: ${MUSL_SHA256}
${LIBSTDCXX_APK}: ${LIBSTDCXX_SHA256}
${LIBGCC_APK}: ${LIBGCC_SHA256}
Instalado por MosaicOS para ejecutarse con qemu-x86_64 (qemu-user i386).
EOF
chmod 0644 "$PREFIX/MANIFEST"

log "OpenCode ${OPENCODE_VERSION} instalado en $PREFIX"
