# Mosaic Compatibility Engine — 0.45

Primera utilidad de inventario, de solo lectura. Ejecuta `compatibility/tools/mosaic-compat status` para detectar arquitectura, sistema, memoria, GPU y herramientas de compatibilidad.

## Límites deliberados
- No instala paquetes, no cambia arquitectura multiarch y no ejecuta archivos recibidos.
- Wine es la capa general de compatibilidad Windows; Proton está orientado principalmente a juegos gestionados por Steam.
- Bottles administra entornos Wine, pero su instalación y dependencias deben evaluarse por perfil.
- DXVK requiere una GPU/controlador con Vulkan adecuado. No se debe forzar en hardware antiguo sin Vulkan.
- La detección de una herramienta no demuestra compatibilidad de una aplicación. Mantener resultados por aplicación en `tests/app-compatibility.csv`.

En equipos con 1 GB, las capas de compatibilidad son opcionales y bajo demanda. No forman parte de la instalación mínima.
