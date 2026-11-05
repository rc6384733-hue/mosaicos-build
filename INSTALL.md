# Construcción, prueba e instalación — MosaicOS 0.50 Aero (i386)

## Requisitos

- CPU de 64 bits para compilar (recomendado) o un host i386 nativo.
- Debian 12 Bookworm o Debian 13 Trixie para la ruta nativa.
- Docker Desktop para la ruta portable en Windows/macOS.
- Conexión a Internet.
- **20 GiB libres como mínimo; 30 GiB recomendados.**
- `root`/`sudo` para `live-build` y Docker.

La ISO resultante es de **32 bits (i386)** y arranca por **BIOS (syslinux/isolinux)**. No se genera arranque UEFI de 32 bits.

## Ruta A — Host Debian

```sh
cd desktop
sudo sh ./prepare-build-host.sh
sudo sh ./build.sh i386
```

`prepare-build-host.sh` instala `live-build`, `debootstrap`, `squashfs-tools`, `xorriso`, syslinux/isolinux, `mtools`, herramientas de imagen y, si el host es amd64, `qemu-user-static`/`binfmt-support` para ejecutar binarios i386 durante la construcción.

El constructor:

- comprueba que el host sea Debian y que existan las herramientas;
- crea `desktop/build/i386`;
- copia la configuración y deja solo el perfil i386;
- ejecuta `lb config` y `lb build`;
- guarda el log en `desktop/build/i386/build.log`;
- copia la ISO y su SHA-256 a `desktop/dist/`.

Si quieres reducir el umbral de espacio (no recomendado):

```sh
sudo sh -c 'MOSAIC_MIN_FREE_GIB=15 ./build.sh i386'
```

## Ruta B — Docker

La imagen `mosaicos-builder` contiene Debian Bookworm y todas las dependencias. El proyecto se monta en `/work`.

**Windows (PowerShell):**

```powershell
powershell -ExecutionPolicy Bypass -File .\desktop\docker\build-with-docker.ps1
```

**Linux/macOS:**

```sh
sh ./desktop/docker/build-with-docker.sh
```

**Manual:**

```sh
docker build -f desktop/docker/Dockerfile -t mosaicos-builder desktop
docker run --rm -v "$PWD:/work" mosaicos-builder i386
```

La salida queda en `desktop/dist/` del equipo anfitrión.

## Ruta C — GitHub Actions

El workflow `.github/workflows/build-iso.yml`:

1. libera espacio en el runner;
2. construye la imagen Docker;
3. ejecuta `tests/validate_050.py`;
4. compila la ISO;
5. publica la ISO, el SHA-256 y el log como artefacto.

Se lanza manualmente desde la pestaña **Actions** o al subir una etiqueta `v*`.

## Verificar la imagen

```sh
sh ./desktop/verify-iso.sh desktop/dist/MosaicOS-0.50-i386.iso
sh ./desktop/check-iso.sh desktop/dist/MosaicOS-0.50-i386.iso
sha256sum -c desktop/dist/MosaicOS-0.50-i386.iso.sha256
```

`verify-iso.sh` identifica el archivo, el volumen y los archivos de arranque. **No sustituye** una prueba de arranque real.

## Probar en QEMU

```sh
qemu-system-i386 -m 2048 -cdrom desktop/dist/MosaicOS-0.50-i386.iso -boot d
```

Recomendaciones:

- Usa primero un disco virtual desechable.
- Prueba 1024, 2048 y 4096 MiB de RAM.
- Comprueba red, audio, resolución, USB y apagado.
- Prueba el instalador solo con un disco virtual vacío.

Para arrancar desde un USB real:

```sh
sudo dd if=desktop/dist/MosaicOS-0.50-i386.iso of=/dev/sdX bs=4M status=progress oflag=sync
```

Sustituye `/dev/sdX` por el dispositivo correcto. **Es destructivo.** En Windows puedes usar Rufus en modo «Imagen DD» o `xfburn`/`usb-creator` desde Linux.

## Instalar en disco

1. Arranca la ISO.
2. Elige «Instalar MosaicOS» en el escritorio o «Instalar» en el menú de arranque.
3. Sigue el instalador Debian.
4. Al terminar, retira el medio y reinicia.

El instalador crea una base Debian. La experiencia MosaicOS completa (tema, panel, herramientas propias) está garantizada en la sesión live; tras la instalación puede requerir configuración adicional. No se presenta como un instalador propio terminado.

## Persistencia en USB (opcional)

1. Crea una partición ext4 etiquetada `persistence`.
2. Añade un archivo `persistence.conf` con `/ home` o la ruta deseada.
3. Arranca y añade `persistence` a las opciones del kernel.

La persistencia no está probada en esta versión.

## Problemas frecuentes

| Síntoma | Causa probable | Solución |
|---|---|---|
| `lb build` falla al descargar paquetes | Red, espejo o suite incorrecta | Revisa `desktop/build/i386/build.log`; usa un espejo Debian accesible |
| Faltan herramientas | Host sin dependencias | Ejecuta `prepare-build-host.sh` o usa Docker |
| Error de emulación i386 en amd64 | `binfmt`/QEMU no activo | `sudo update-binfmts --enable qemu-i386` o usa un host i386 |
| La ISO no arranca en UEFI | Solo hay BIOS (syslinux/isolinux) | Activa modo BIOS/CSM o usa QEMU |
| Wine no abre un `.exe` | Programa de 64 bits o dependencia no soportada | Usa una versión de 32 bits; prueba `winetricks` |
| El instalador no aparece | Falta `debian-installer-launcher` | Arranca y elige «Instalar» en el menú de arranque |

## Qué no está incluido

- Una ISO ya compilada.
- Soporte de aplicaciones Windows de 64 bits.
- Soporte UEFI de 32 bits.
- Un instalador propio que replique todo MosaicOS en el disco.
- Garantía de compatibilidad total con Wine.
- Mediciones de rendimiento en 1 GB de RAM.

No instales esta versión sobre un disco con datos importantes sin una copia de seguridad verificada.
