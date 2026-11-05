# Mosaic Store — catálogo de aplicaciones

El catálogo es una selección de aplicaciones de los repositorios Debian. No contiene binarios redistribuidos por MosaicOS. `mosaic-store` consulta la arquitectura del equipo, muestra solo entradas permitidas para ella y solicita confirmación antes de invocar APT con `--no-install-recommends`.

La edición 0.50 incluye 17 entradas. Los perfiles `ligero`, `medio` y `alto` son orientativos, no mediciones de RAM. La disponibilidad real depende de la suite, arquitectura y repositorios habilitados; APT valida el paquete al instalarlo. Firefox ESR está disponible en amd64 e i386 y puede resultar pesado en equipos con 1 GB de RAM. Netsurf tiene compatibilidad limitada con sitios web modernos.

Entradas de compatibilidad: Wine32, Winetricks, DOSBox y ScummVM. Wine solo ejecuta programas Windows de 32 bits en esta edición.

Comandos: `mosaic-store list`, `mosaic-store search TEXTO`, `mosaic-store info ID`, `mosaic-store install ID`.
