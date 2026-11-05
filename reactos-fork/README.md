# MosaicOS NT — línea ReactOS-derived

## Objetivo de esta rama

Partir del código real de ReactOS y modificarlo directamente, sin fingir que ya existe un kernel NT propio de MosaicOS. La primera etapa debe concentrarse en una derivación pequeña, compilable y fácil de comparar con upstream.

### Identidad de producto

- Nombre visible propuesto: **MosaicOS**.
- Nombre interno de esta rama: **MosaicOS NT**.
- Identificador de distribución: `MOSAICOS`.
- Mantener atribuciones y avisos de ReactOS donde corresponda.
- No usar logotipos, marcas ni recursos de Microsoft/Windows como si fueran propios.

### Orden de trabajo

1. Obtener ReactOS upstream y registrar el commit exacto usado.
2. Compilar ReactOS sin cambios y probar la ISO en QEMU.
3. Crear una rama Git `mosaicos/main` y guardar la receta de build.
4. Añadir identidad MosaicOS en recursos propios, pantalla de inicio, nombre del sistema y documentación, sin romper recursos compartidos.
5. Personalizar shell/escritorio y configuración como cambios separados y reversibles.
6. Añadir pruebas de arranque, instalación, red, almacenamiento, audio, vídeo y aplicaciones.
7. Mantener un registro de compatibilidad por hardware y aplicación.

## No prometer todavía

ReactOS no equivale a compatibilidad total con Windows. La compatibilidad de aplicaciones y drivers varía. No hay soporte confirmado para todos los equipos modernos, UEFI, software actual ni arquitecturas distintas de las que el upstream soporte.

## Estructura

- `UPSTREAM.md`: procedencia y referencia oficial.
- `LICENSING.md`: reglas de distribución y avisos.
- `branding/`: textos y metadatos propios de MosaicOS.
- `patches/README.md`: política para parches pequeños y revisables.
- `tools/`: scripts de clonación y compilación.
