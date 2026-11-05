# MosaicOS 0.50 Aero (i386)

MosaicOS 0.50 Aero es una **edición de escritorio real de 32 bits**, basada en Debian GNU/Linux 12 (Bookworm). No es una página web ni un escritorio simulado: arranca un kernel Linux, inicia una sesión gráfica LXQt con Openbox y ejecuta aplicaciones nativas.

La imagen se distribuye como **receta de compilación**. Este ZIP contiene el código y la configuración, pero **no incluye una ISO ya compilada**. Las instrucciones para generarla están más abajo.

## Qué incluye

- **Sistema:** Debian 12 Bookworm i386, kernel `686-pae`, arranque BIOS con syslinux/isolinux.
- **Escritorio:** LXQt + Openbox con tema Aero inspirado en Windows 7, panel inferior, orbe de inicio, fondos y sonido de arranque propios.
- **Aplicaciones Linux nativas:** Firefox ESR, Thunderbird, LibreOffice, VLC, MPV, GIMP, Inkscape, Audacious, qBittorrent/Transmission, FileZilla, Remmina, GParted, Synaptic y más.
- **Aplicaciones Windows:** Wine32 + `winetricks` + integración de archivos `.exe`. Funcionan programas **Windows de 32 bits**; los de 64 bits no pueden ejecutarse en un sistema i386.
- **Instalación:** lanzador del instalador Debian incluido en la ISO y acceso directo «Instalar MosaicOS».
- **Mantenimiento:** Centro Mosaic, Configuración Mosaic, diagnóstico, informes, copias de seguridad, actualizaciones y Mosaic Store.
- **Idioma:** español (locale `es_ES.UTF-8`, teclado `es`, zona horaria `Europe/Madrid`).

## Compilar la ISO

### Opción A — Debian 12/13 real

```sh
cd desktop
sudo sh ./prepare-build-host.sh
sudo sh ./build.sh i386
```

Si descomprimiste el ZIP en Linux y los scripts no tienen permiso de ejecución, `sh` evita el problema.

La ISO se genera en `desktop/dist/MosaicOS-0.50-i386.iso` si la compilación termina correctamente.

### Opción B — Windows o macOS con Docker Desktop

```powershell
powershell -ExecutionPolicy Bypass -File .\desktop\docker\build-with-docker.ps1
```

En Linux/macOS con Docker:

```sh
sh ./desktop/docker/build-with-docker.sh
```

### Opción C — GitHub Actions

El flujo `.github/workflows/build-iso.yml` compila la ISO en un runner de GitHub y la publica como artefacto. Se puede lanzar manualmente o al crear una etiqueta `v*`.

## Probar la ISO

```sh
qemu-system-i386 -m 2048 -cdrom desktop/dist/MosaicOS-0.50-i386.iso -boot d
```

Comprueba primero en QEMU con un disco virtual desechable. El arranque es **BIOS (syslinux/isolinux)**; UEFI de 32 bits no está soportado. Una compilación correcta no demuestra por sí sola que la imagen arranque: hay que probarla.

## Estado real

- Receta, configuración de escritorio, tema, herramientas y validadores: implementados.
- Validación estática local: superada.
- Disponibilidad de los 227 paquetes en Bookworm i386: comprobada contra `packages.debian.org`.
- **ISO compilada: no incluida en este ZIP.**
- **Arranque en QEMU/hardware real: pendiente de ejecutar en un host con Linux/Docker.**
- La compatibilidad con programas Windows depende de cada aplicación y no es universal.

Consulta `PROJECT_STATUS.md`, `INSTALL.md`, `QUICKSTART.md` y `BUILD_REPORT-0.50.txt` para el detalle.

## Estructura

```
desktop/                 receta live-build, configuración y herramientas
  build.sh               compilación de la ISO i386
  prepare-build-host.sh  preparación de un host Debian
  config/                paquetes, hooks, escritorio, tema y utilidades
  docker/                compilación reproducible con Docker
tests/validate_050.py    validación estática de la receta 0.50
.github/workflows/       compilación de la ISO en GitHub Actions
```
