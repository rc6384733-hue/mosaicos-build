# Compilación de MosaicOS 0.50 Aero (i386)

Esta carpeta contiene la receta completa de la ISO de 32 bits: selección de paquetes, configuración de live-build, escritorio LXQt/Openbox con tema Aero, integración de Wine32 y herramientas Mosaic.

## Requisitos del host

- Debian 12 Bookworm o Debian 13 Trixie, amd64 o i386.
- `root`/`sudo`, conexión a Internet y 20–30 GiB libres.
- Para compilar i386 desde amd64: `qemu-user-static` y `binfmt-support`.

## Compilación nativa

```sh
sudo sh ./prepare-build-host.sh
sudo sh ./build.sh i386
```

Salida esperada:

```
dist/MosaicOS-0.50-i386.iso
dist/MosaicOS-0.50-i386.iso.sha256
build/i386/build.log
```

## Compilación con Docker

Desde la raíz del proyecto:

```sh
docker build -f desktop/docker/Dockerfile -t mosaicos-builder desktop
docker run --rm -v "$PWD:/work" mosaicos-builder i386
```

En Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\desktop\docker\build-with-docker.ps1
```

## Estructura de `config`

```
config/
  package-lists/mosaic-desktop-i386.packages   paquetes de la ISO
  hooks/live/0100-mosaic-configuration.hook.chroot
  includes.chroot/
    etc/                 locale, teclado, LightDM, sudoers, skel, autostart
    usr/local/bin/       herramientas Mosaic y lanzadores de Wine
    usr/share/           fondos, iconos, tema Openbox, sonidos, catálogo
```

`build.sh` copia `config` al workspace `build/i386`, elimina cualquier perfil de otra arquitectura y deja solo `mosaic-desktop-i386.packages` como `mosaic-desktop.list.chroot`. Así se evita mezclar dependencias de amd64 y i386.

## Opciones principales de live-build

- `--distribution bookworm`
- `--architectures i386`
- `--binary-images iso-hybrid`
- `--archive-areas "main contrib non-free non-free-firmware"`
- `--linux-flavours 686-pae`
- `--bootloaders syslinux` (BIOS, isolinux)
- `--debian-installer live`
- `--bootappend-live` con hostname, usuario, locale, teclado, zona horaria y `quiet splash`

## Escritorio

- **LXQt + Openbox** en la sesión `lxqt`.
- Panel inferior configurado en `etc/skel/.config/lxqt/panel.conf`.
- Tema Openbox `MosaicAero` en `usr/share/themes/MosaicAero/openbox-3/`.
- Iconos y orbe de inicio en `usr/share/icons/MosaicAero/`.
- Fondos en `usr/share/backgrounds/mosaicos/`.
- Compositor picom en `etc/xdg/picom.conf` y autostart.
- LightDM con autologin y fondo de acceso.

Los archivos de `etc/skel` se copian al crear el usuario. El hook del chroot crea/ajusta el usuario `mosaic`, permisos, locale, teclado, zona horaria, servicios y cachés.

## Compatibilidad Windows

- `mosaic-wine-setup` crea un prefijo Win32 y configura Windows 7.
- `mosaic-run-exe` abre `.exe`, `.msi`, `.bat` y `.cmd`.
- Detecta PE32+ y rechaza programas de 64 bits.
- `mimeapps.list` y la acción de PCManFM-Qt permiten abrir `.exe` con doble clic.
- No se incluye `wine64`: no es utilizable en i386.

## Validación

```sh
python3 tests/validate_050.py
```

Comprueba versión, opciones de la receta, perfil de paquetes, recursos gráficos, configuración del panel, tema de Openbox, entradas `.desktop`, atajos, hook, sonido, Docker y CI. No construye ni arranca la ISO.

Disponibilidad de paquetes comprobada contra `https://packages.debian.org/bookworm/i386/`: 227/227 encontrados.

## Verificación y prueba

```sh
sh ./verify-iso.sh dist/MosaicOS-0.50-i386.iso
qemu-system-i386 -m 2048 -cdrom dist/MosaicOS-0.50-i386.iso -boot d
```

Una compilación correcta no demuestra que la imagen arranque. Prueba en QEMU y en hardware real antes de instalar.

## Limitaciones

- Solo BIOS; no UEFI de 32 bits.
- Solo aplicaciones Windows de 32 bits.
- La compatibilidad con Wine depende de cada programa.
- Las personalizaciones tras la instalación en disco requieren trabajo adicional.
- El escritorio web antiguo (`desktop/win7/`) es histórico y no se incluye en la ISO.
