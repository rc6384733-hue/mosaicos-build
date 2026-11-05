# Investigación de proyectos abiertos — corte 0.47

Esta revisión es preliminar y documental. No se copiaron fuentes de terceros en esta versión; se tomaron decisiones de diseño y se registraron riesgos antes de integrar código externo.

## Referencias revisadas

### Debian Live / live-build
- Fuente: https://live-team.pages.debian.net/live-manual/html/live-manual/examples.en.html
- Uso considerado: construir imágenes live desde un directorio de configuración, con paquetes, hooks y overlays.
- Decisión: mantener `live-build` como constructor de MosaicOS Daily; aislar el estado por arquitectura y preservar las ISO en `desktop/dist/`.
- Licencia/procedencia: herramienta del proyecto Debian Live; revisar avisos de cada paquete y scripts antes de redistribuir.

### nodiscc/debian-live-config
- Fuente: https://github.com/nodiscc/debian-live-config
- Aporta un ejemplo de distribución Debian live enfocada en equipos de escritorio y hardware reciclado, usando `live-build` y pocos componentes de terceros.
- Decisión: referencia de organización de configuración y enfoque de bajo mantenimiento; no se copió código ni se afirma compatibilidad automática con la licencia de ese repositorio. GitHub indica que es un espejo y señala Codeberg como otra ubicación del proyecto.
- Estado de licencia: el repositorio enlaza un archivo LICENSE; antes de reutilizar archivos concretos hay que leer el texto y preservar atribuciones.

### minios-linux/minios-live
- Fuente: https://github.com/minios-linux/minios-live
- Licencia declarada por GitHub: GPL-2.0.
- Aporta un constructor por etapas y opciones para arquitecturas amd64/i386, entornos LXQt y variantes de paquetes.
- Riesgo observado en su documentación: describe un bootstrap con `debootstrap --no-check-gpg` y descarga de algunas claves por HTTP no autenticado.
- Decisión: no integrar ni copiar el bootstrap. Sí queda como referencia para evaluar construcción por etapas, pero MosaicOS debe exigir verificación de procedencia y autenticidad de sus entradas.

### LXQt
- Fuente: https://github.com/lxqt/lxqt
- Uso considerado: escritorio modular para Mosaic Experience.
- Decisión: mantener LXQt como componente de escritorio upstream, personalizando configuración y herramientas Mosaic; no forzar una reescritura propia del escritorio.
- Licencias upstream anunciadas por el proyecto: GPL-2.0+ y LGPL-2.1+; verificar la licencia de cada subcomponente al redistribuir.

## Próximas comprobaciones antes de integrar código
1. Fijar commit o versión exacta.
2. Leer LICENSE y avisos de todos los subdirectorios que se reutilicen.
3. Revisar actividad, issues de seguridad y dependencias.
4. Construir sin modificaciones y registrar hashes.
5. Aplicar cambios Mosaic en parches pequeños, reversibles y documentados.
6. Repetir build y pruebas en QEMU.

No se considera un proyecto aprobado solo por tener muchas estrellas o por ofrecer una ISO.
