# Mosaic Compatibility Engine — plan inicial

## Orden de integración

1. Inventario del ejecutable y arquitectura (PE x86/x64/ARM64), sin ejecutarlo.
2. Base de datos local de resultados de compatibilidad.
3. Wine en amd64 como primera vía para aplicaciones Windows compatibles.
4. Bottles como interfaz opcional para entornos separados, si sus dependencias y consumo encajan.
5. Proton para juegos compatibles mediante Steam; no tratarlo como reemplazo general de Wine.
6. DXVK/VKD3D-Proton solo cuando la GPU, Vulkan y controladores lo permitan.

## Reglas

- Nada de ejecutar automáticamente archivos descargados o desconocidos.
- Cada aplicación se ejecuta con usuario sin privilegios y prefijo separado cuando sea posible.
- Mostrar dependencias, limitaciones y permisos antes de iniciar.
- No prometer compatibilidad universal.
- La edición 32-bit no incluirá Proton como expectativa: el ecosistema moderno de juegos y controladores suele exigir 64 bits y GPU compatible.
- Medir RAM y CPU antes de decidir qué componentes van preinstalados.
