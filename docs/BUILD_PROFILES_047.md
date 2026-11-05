# Perfiles de compilación 0.47

> **Documento histórico.** A partir de MosaicOS 0.50 la edición es exclusivamente **i386 / Debian 12 Bookworm**. El perfil amd64 no se compila y se conserva solo como referencia.

| Perfil | Base | Objetivo | Estado |
|---|---|---|---|
| amd64 | Debian 13 Trixie | PC x86-64 | Receta estática; ISO no construida |
| i386 | Debian 12 Bookworm | PC x86 de 32 bits | Receta estática legacy; ISO no construida |

Cada perfil tiene una lista de paquetes independiente y su propio directorio `desktop/build/<arquitectura>`. Las imágenes se copian a `desktop/dist/`.

El host de compilación debe ser Debian amd64. El perfil i386 se construye apuntando a Bookworm; el hecho de compilar desde amd64 no garantiza por sí solo que cada paquete i386 esté disponible. La disponibilidad debe confirmarse contra los índices de Bookworm en el host real.
