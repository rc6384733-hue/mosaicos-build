# Cambios MosaicOS 0.50 Aero (i386)

## Cambio principal

- El proyecto deja de ser un escritorio web y pasa a ser una **ISO de escritorio nativa de 32 bits** basada en Debian 12 Bookworm.
- Se elimina el escritorio web del proceso de compilación. `desktop/win7/` queda como material histórico y no se incluye en la imagen.
- La edición 0.50 es exclusivamente **i386**; el constructor rechaza amd64.

## Escritorio

- Sesión LXQt + Openbox con panel inferior, menú de inicio, barra de tareas, bandeja y reloj.
- Tema `MosaicAero` inspirado en Windows 7: barra de título con degradado, botones XBM de cerrar/maximizar/minimizar y menú.
- Tema de iconos `MosaicAero` con orbe de inicio en 8 tamaños.
- Fondos de escritorio y de acceso en 1920×1080.
- Sonido de inicio WAV sintetizado.
- Compositor picom para sombras y transiciones.
- Configuración en español: locale, teclado, zona horaria, carpetas personales y textos de herramientas.

## Aplicaciones y sistema

- Perfil de 227 paquetes para uso diario: Firefox ESR, Thunderbird, LibreOffice, VLC, MPV, GIMP, Inkscape, Audacity, FileZilla, Remmina, GParted, Synaptic, GDebi, Timeshift, KeePassXC, impresión, Bluetooth, energía y utilidades.
- Kernel `686-pae` con PAE, firmware Wi-Fi/GPU/sonido y microcódigo.
- Soporte de VM con QXL, `spice-vdagent` y `open-vm-tools-desktop`.
- `zram-tools` y `earlyoom` para equipos con poca memoria.
- LightDM con autologin y fondo de acceso.
- Instalador Debian live accesible desde el menú de arranque y el escritorio.

## Compatibilidad Windows

- Wine32 preinstalado con prefijo Win32 y modo Windows 7.
- `mosaic-run-exe` abre `.exe`, `.msi`, `.bat` y `.cmd`.
- Detección y rechazo explícito de programas Windows de 64 bits.
- Asociaciones MIME y acción de PCManFM-Qt para ejecutar `.exe` con doble clic.
- `winetricks`, `cabextract`, DOSBox y ScummVM incluidos.

## Compilación y validación

- `desktop/build.sh` reescrito para i386 con workspace aislado, control de espacio y SHA-256.
- `desktop/prepare-build-host.sh` actualizado para hosts Debian amd64/i386 y emulación QEMU.
- Nuevo `desktop/docker/` con Dockerfile, entrada y scripts para Windows/Linux.
- Nuevo workflow `.github/workflows/build-iso.yml`.
- Nuevo validador `tests/validate_050.py`.
- Catálogo Mosaic Store ampliado a 17 entradas, con Firefox ESR para i386 y aplicaciones de compatibilidad.

## Limitaciones declaradas

- No se incluye una ISO compilada.
- Solo aplicaciones Windows de 32 bits.
- Arranque BIOS; sin UEFI de 32 bits.
- La instalación en disco usa el instalador Debian y no replica automáticamente todas las personalizaciones.
- La compatibilidad con Wine no es universal.

# Cambios MosaicOS 0.49

- Incorpora Mosaic Store, catálogo curado de aplicaciones con filtro por arquitectura.
- Añade búsqueda, ficha informativa e instalación explícita mediante APT sin recomendaciones.
- Añade una selección inicial de aplicaciones opcionales: Netsurf, Firefox ESR (amd64), AbiWord, Gnumeric, MPV, Audacious, Ristretto, Geany, Git, GParted, Synaptic y FileZilla.
- Preinstala Python 3, Audacious y MPV en ambos perfiles; el resto permanece opcional.
- Actualiza el lanzador gráfico del catálogo y su documentación.
- No se afirma que todas las aplicaciones estén disponibles en todos los repositorios ni que funcionen con 1 GB de RAM; APT valida disponibilidad durante la instalación.
