# OpenCode en MosaicOS 0.50 i386

MosaicOS 0.50 Aero es un sistema Debian 12 de 32 bits (i386). OpenCode **no
publica binarios Linux para 32 bits**: los paquetes oficiales solo incluyen
`x64` y `arm64`. Para que OpenCode funcione en un equipo i386 se usa el binario
oficial **x86_64 con libc musl** ejecutado mediante **qemu-user i386**.

No es una versión nativa de 32 bits ni una aplicación web sustituta: es el
mismo binario oficial de OpenCode 1.19.5, ejecutado con emulación de CPU.

## Cómo está integrado

| Pieza | Ruta |
| --- | --- |
| Binario oficial x86_64 (musl) | `/opt/opencode/opencode` |
| Cargador musl x86_64 | `/opt/opencode/x86_64/lib/ld-musl-x86_64.so.1` |
| libc musl x86_64 | `/opt/opencode/x86_64/lib/libc.musl-x86_64.so.1` |
| libstdc++ / libgcc x86_64 | `/opt/opencode/x86_64/usr/lib/` |
| Emulador (proceso i386) | `/usr/bin/qemu-x86_64` (`qemu-user`) |
| Lanzador de consola | `/usr/local/bin/opencode` |
| Lanzador gráfico | `/usr/local/bin/mosaic-opencode` |
| Diagnóstico | `/usr/local/bin/mosaic-opencode-diagnose` |
| Configuración gestionada | `/etc/opencode/opencode.json` |
| Menú y Escritorio | `mosaic-opencode.desktop`, `OpenCode.desktop` |

`opencode` es un *script* que ejecuta:

```sh
qemu-x86_64 -cpu max \
  -E LD_LIBRARY_PATH=/opt/opencode/x86_64/lib:/opt/opencode/x86_64/usr/lib \
  /opt/opencode/x86_64/lib/ld-musl-x86_64.so.1 \
  /opt/opencode/opencode "$@"
```

Se invoca el cargador musl de forma explícita porque `qemu-x86_64 -L` reescribe
todas las rutas absolutas del invitado y rompería el acceso a `/proc`, `/home`
y al propio binario.

## Uso

Desde una terminal:

```sh
opencode
```

Desde el menú de aplicaciones o el icono **OpenCode** del Escritorio se abre
`qterminal` con OpenCode.

La primera vez hay que configurar un proveedor y una clave de API. OpenCode
guarda las credenciales en el perfil del usuario. La configuración gestionada
de `/etc/opencode/opencode.json` solo desactiva las actualizaciones automáticas:
el binario x86_64 no se puede sustituir por una actualización de 64 bits.

### Modo de emergencia

En un Atom con 2 GB de RAM, la emulación y JavaScriptCore pueden quedarse sin
memoria o fallar al reservar direcciones. El lanzador ya reduce las reservas de
JIT, pero si OpenCode no arranca se puede probar:

```sh
MOSAIC_OPENCODE_SAFE=1 opencode --version
MOSAIC_OPENCODE_SAFE=1 opencode
```

Este modo desactiva el JIT (`BUN_JSC_useJIT=0`). Es bastante más lento, pero usa
menos memoria y evita las asignaciones ejecutables grandes.

Para ver qué falta o si el binario arranca:

```sh
sudo mosaic-opencode-diagnose
```

## Por qué no hay un binario nativo i386

- `opencode-ai` y sus paquetes de plataforma publican únicamente `cpu: x64` y
  `cpu: arm64`.
- OpenCode se distribuye como un ejecutable autocontenido de Bun. Bun tampoco
  ofrece compilación para Linux ia32, así que no existe una vía nativa de
  32 bits ni compilando desde el código fuente.
- La alternativa realista es emular x86_64 con `qemu-user` i386. El emulador
  es un proceso de 32 bits y encaja en un sistema i386 sin kernel de 64 bits.

## Limitaciones

- El rendimiento es el de una CPU emulada por software. Arrancar OpenCode puede
  tardar decenas de segundos o varios minutos en un Atom.
- Es un componente **experimental**. La prueba automática de CI comprueba que
  `opencode --version` y `opencode --help` funcionan con `qemu-x86_64` de 32
  bits, pero no sustituye a una sesión interactiva larga en el netbook.
- Las herramientas que OpenCode lanza (shell, git, ripgrep) deben ser nativas
  i386 o binarios x86_64 ejecutables por qemu. El sistema incluye git y las
  utilidades habituales de Debian i386.
- No se admiten actualizaciones automáticas de OpenCode: la versión está fijada
  y verificada por SHA-256.
- Requiere conexión a Internet y una cuenta/clave del proveedor de modelos.

## Verificación y reconstrucción

La prueba de CI (`tests/opencode-i386/run.sh`) instala `qemu-user:i386` en un
contenedor amd64. Así `qemu-x86_64` se ejecuta como proceso i386 nativo (sin
emulación anidada) y reproduce el límite de 4 GiB de un sistema de 32 bits.
Después descarga los artefactos oficiales, verifica sus SHA-256 e intenta
arrancar OpenCode.

El instalador se puede repetir en un sistema ya arrancado:

```sh
sudo sh /usr/local/lib/mosaicos/opencode-install.sh
```

Artefactos y sumas fijadas:

| Artefacto | SHA-256 |
| --- | --- |
| `opencode-linux-x64-baseline-musl-1.19.5.tgz` | `1f4c1fcc646d4d03d1ea0997a54d1e8b9fe17af6356ac54dde2c65e53fedc682` |
| `package/bin/opencode` | `528058cf8f2a7196ec94dc113052b785f8f94f6802ced3980ad7359369ddacc8` |
| `musl-1.2.5-r4.apk` | `b255cdcf4f64eb301f4dbd40d1fbdc0fcb021613b3f0d124a41eeff62e2bc7d8` |
| `libstdc++-13.2.1_git20240309-r1.apk` | `af0fe894ef5051116e321bf4753a10fffa85abc2b71f30b2e949467775421ace` |
| `libgcc-13.2.1_git20240309-r1.apk` | `f348d99e10b5267566afe6f80861661b08cb5aa43a6d4d1c8f1792b5001d0995` |
