# Auditoría de base — MosaicOS 0.46

## Decisión provisional

| Línea | Base | Motivo | Riesgo / límite |
|---|---|---|---|
| amd64 | Debian 13 (trixie) | Línea actual con soporte moderno y arquitectura principal | Navegadores y aplicaciones modernas pueden exceder 1 GB |
| i386 | Debian 12 (bookworm) | Última generación Debian con soporte instalable para equipos x86 de 32 bits | Línea legacy; disponibilidad futura de paquetes y navegadores más limitada |

## Evidencia consultada

- Debian declara que i386 ya no es una arquitectura regular en Debian 13 y que no hay kernel oficial ni instalador para equipos exclusivamente i386: https://www.debian.org/ports/i386/
- antiX 23 publica imágenes Bookworm tanto de 32 como de 64 bits, incluida una variante base cercana a 1 GB de descarga: https://antixlinux.com/antix-23-released/
- El manual de Debian Live documenta el uso de live-build, debootstrap y la construcción de sistemas live personalizados: https://live-team.pages.debian.net/live-manual/html/live-manual.en.html
- MiniOS publica un constructor por etapas y documenta perfiles i386/amd64 y escritorios ligeros; se registra como alternativa a evaluar, no como dependencia incorporada: https://github.com/minios-linux/minios-live

## Conclusión

No es correcto compilar ambas ediciones contra Debian 13. MosaicOS 0.46 separa amd64 (Trixie) e i386 (Bookworm). La edición i386 queda marcada como legacy, y no se promete el mismo horizonte de soporte que amd64. antiX y MiniOS quedan como alternativas a comparar en pruebas, no se copian ni redistribuyen sus binarios sin auditoría de licencia y procedencia.

## Pendiente de validación

- Compilar ambas ISO desde un host Debian amd64 limpio.
- Verificar el arranque en QEMU con 1 GB asignado.
- Medir RAM en reposo y con aplicaciones reales.
- Comprobar red, audio, USB, vídeo y suspensión en hardware representativo.
- Auditar licencias y dependencias de cada componente que se incorpore.
