# Estado del proyecto — MosaicOS 0.50 Aero (i386)

## Implementado en el código

- Edición única de 32 bits sobre Debian 12 Bookworm i386, kernel `686-pae`.
- ISO live híbrida con arranque BIOS (syslinux/isolinux) e instalador Debian live.
- Escritorio LXQt + Openbox con tema Aero de Windows 7: fondos, orbe de inicio, botones de ventana, panel inferior, sonido de arranque y compositor picom.
- Configuración en español: locale, teclado, zona horaria, carpetas personales y textos de las herramientas.
- Conjunto de 227 paquetes orientado a uso diario: navegación, correo, oficina, multimedia, gráficos, redes, impresión, Bluetooth, energía, copias de seguridad y utilidades.
- Compatibilidad con programas Windows de 32 bits mediante Wine32, prefijo Win32 configurado como Windows 7, asociaciones `.exe`, `winetricks` y rechazo explícito de binarios PE32+ de 64 bits.
- Instalador accesible desde el menú de arranque y desde el escritorio.
- Herramientas Mosaic: bienvenida gráfica, Centro Mosaic, Configuración, Store, diagnóstico, informes, copias de seguridad, actualizaciones y datos del equipo.
- Rutas de compilación: host Debian, contenedor Docker y GitHub Actions.
- Validador estático `tests/validate_050.py`.

## Validado localmente

- `tests/validate_050.py`: superado (receta, paquetes, recursos, escritorio, Wine32, hook, Docker y CI).
- `mosaic-store`: compilación de sintaxis Python correcta.
- Catálogo CSV: 17 entradas, filtrado por arquitectura.
- Disponibilidad de los 227 paquetes del perfil i386 en `packages.debian.org/bookworm/i386`: 227/227 encontrados.
- Coherencia de nombres de paquetes prohibidos (`wine64`, `virtualbox-guest-*`): ausentes.
- Integridad de los recursos generados: PNG, XBM, `index.theme`, `startup.wav` y archivos `.desktop`.

## Pendiente de probar en el sistema objetivo

- Compilación real de la ISO con `live-build`.
- Arranque en QEMU y en hardware real.
- Sesión LXQt/Openbox, panel, tema, red, audio, vídeo y apagado.
- Instalación en disco desde el instalador Debian live.
- Instalación y ejecución de aplicaciones Linux desde Mosaic Store.
- Creación del prefijo Wine y ejecución de programas Windows de 32 bits concretos.
- Consumo de RAM/CPU y tiempos de arranque.
- Persistencia en USB.

## Limitaciones conocidas

- **No hay ISO compilada en este ZIP.** El entorno de trabajo original era Windows sin Linux, WSL, Docker ni Python; la receta no se pudo ejecutar allí.
- **Solo programas Windows de 32 bits.** Un sistema i386 no puede ejecutar binarios Windows x86-64.
- La compatibilidad con Wine no es universal; depende de la aplicación.
- Arranque **BIOS únicamente**; no se genera arranque UEFI de 32 bits.
- La instalación usa el instalador Debian live. Las personalizaciones de MosaicOS están garantizadas en la sesión live; un instalador propio que replique todo el escritorio en el disco sigue pendiente.
- El rendimiento con 1 GB de RAM no está medido; Firefox, LibreOffice y Wine pueden necesitar más memoria.
