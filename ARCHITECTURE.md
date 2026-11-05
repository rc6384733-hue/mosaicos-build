# Arquitectura — MosaicOS 0.50 Aero (i386)

## Visión general

MosaicOS 0.50 es una distribución de escritorio de 32 bits construida con `live-build` sobre Debian 12 Bookworm. Debian aporta kernel, controladores, systemd, red, audio y APT. MosaicOS aporta la selección de paquetes, la experiencia de escritorio, las herramientas propias y la integración de aplicaciones Windows.

La edición 0.50 sustituye el antiguo escritorio web por una sesión gráfica nativa. El directorio `desktop/win7/` se conserva como material histórico y **no se incluye en la ISO**.

## Capas

1. **Hardware y kernel**
   - Kernel `linux-image-686-pae` para CPUs de 32 bits con PAE.
   - Firmware libre y no libre (`firmware-iwlwifi`, `firmware-realtek`, `firmware-atheros`, `firmware-brcm80211`, `firmware-amd-graphics`, etc.).
   - `microcode` de Intel cuando el hardware lo permite.
   - Soporte de VM: `spice-vdagent`, `open-vm-tools-desktop`, driver QXL.

2. **Base Debian**
   - `systemd`, APT, NetworkManager, PulseAudio, CUPS, UDisks2/GVFS y PolicyKit.
   - Repositorios `main`, `contrib`, `non-free` y `non-free-firmware`.
   - Instalador Debian live (`debian-installer-launcher`).

3. **Sesión gráfica**
   - LightDM con autologin del usuario live `mosaic`.
   - LXQt como entorno de escritorio y Openbox como gestor de ventanas.
   - Picom como compositor para sombras y transiciones.
   - Panel inferior LXQt con menú de inicio (orbe), lanzadores, barra de tareas, bandeja, reloj y botón de escritorio.

4. **Experiencia Mosaic**
   - Tema Openbox `MosaicAero` con botones XBM y degradados azules.
   - Tema de iconos `MosaicAero` con orbe de inicio en 16–256 px.
   - Fondos `mosaic-7.png` y `mosaic-7-login.png`.
   - Sonido de inicio sintetizado en `/usr/share/sounds/mosaic/startup.wav`.
   - Configuración GTK 2/3, Qt5, cursor DMZ, tipografías Noto/Liberation y carpetas en español.
   - Atajos: `Super+E` archivos, `Super+R` ejecutar, `Super+T` terminal, `Super+D` escritorio, `Super+L` bloquear, `Super+W` aplicaciones Windows.

5. **Herramientas Mosaic**
   - `mosaic-welcome` / `mosaic-welcome-gui`: bienvenida e información del sistema.
   - `mosaic-center`: centro de herramientas.
   - `mosaic-settings`: accesos a configuración del sistema.
   - `mosaic-store`: catálogo APT con confirmación explícita.
   - `mosaic-health`, `mosaic-report`, `mosaic-hardware`: diagnóstico de solo lectura.
   - `mosaic-backup`, `mosaic-update`: copias y actualizaciones con autorización.
   - `mosaic-windows`, `mosaic-run-exe`, `mosaic-wine-setup`: compatibilidad Windows.
   - `mosaic-install-disk`: lanzador del instalador Debian.

6. **Compatibilidad Windows**
   - `wine` + `wine32` de Bookworm; nunca `wine64`.
   - Prefijo por usuario en `~/.wine` con `WINEARCH=win32`.
   - Versión de Windows configurada como `win7` para mejorar compatibilidad con aplicaciones antiguas.
   - `WINEDLLOVERRIDES=mscoree,mshtml=` para evitar dependencias de .NET/IE no disponibles.
   - Asociación MIME de `.exe`, `.msi`, `.bat` y `.cmd` con `mosaic-run-exe`.
   - Detección de binarios PE32+ y mensaje claro: no se ejecutan en i386.
   - `winetricks`, `cabextract`, DOSBox y ScummVM para casos adicionales.

## Flujo de arranque

1. syslinux/isolinux carga el kernel `686-pae` y el initrd.
2. `live-boot` monta la ISO y `live-config` prepara la sesión live.
3. Se aplican los parámetros: hostname `mosaicos`, usuario `mosaic`, locale español, teclado español y zona `Europe/Madrid`.
4. systemd arranca red, audio, Bluetooth, impresión y LightDM.
5. LightDM inicia la sesión LXQt de `mosaic` con Openbox.
6. Los autostart configuran Wine, picom, nm-tray, carpetas personales, sonido y bienvenida.

El menú de arranque incluye una entrada de instalación que lanza el instalador Debian live.

## Compilación

`desktop/build.sh`:

1. Comprueba root, Debian, arquitectura del host, herramientas y espacio libre.
2. Crea un workspace aislado en `desktop/build/i386`.
3. Copia `desktop/config` y deja **solo** el perfil `mosaic-desktop-i386.packages`.
4. Ejecuta `lb config` con distribución `bookworm`, arquitectura `i386`, kernel `686-pae`, imagen `iso-hybrid`, syslinux/isolinux e instalador live.
5. Ejecuta `lb build`, guarda el log y copia la ISO a `desktop/dist/`.
6. Genera `MosaicOS-0.50-i386.iso.sha256`.

En un host amd64 se usa `qemu-user-static`/`binfmt-support` para ejecutar los binarios i386 durante el `debootstrap`.

## Instalación

La ISO incluye el instalador Debian. El usuario puede instalar un sistema Debian base en disco. Las personalizaciones MosaicOS están garantizadas en la sesión live; tras la instalación, las herramientas y el tema pueden requerir un paso de post-instalación o un instalador propio, todavía pendiente. Esto se documenta como limitación, no como función terminada.

## Seguridad y mantenimiento

- `sudo` sin contraseña **solo** para el usuario live `mosaic`; en un sistema instalado debe revisarse.
- `ufw` y `gufw` incluidos, desactivados por defecto.
- `firejail` disponible para aislamiento opcional.
- `timeshift`, `grsync` y `rsync` para copias.
- `keepassxc` y `gnome-keyring` para credenciales.
- No se afirma que el sistema sea hardened ni apto para producción.

## Relación con Mosaic Core

`boot.s`, `kernel.c` y `mosaicos.bin` pertenecen al kernel experimental Mosaic Core y siguen siendo un proyecto separado. No forman parte de la ISO de escritorio 0.50.
