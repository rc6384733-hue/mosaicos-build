# Estado de compilación de MosaicOS NT

Esta carpeta contiene automatización de preparación, verificación y compilación; no contiene el árbol fuente upstream.

## Estado en la entrega 0.37

- [x] Verificación de scripts y estructura del kit.
- [x] Verificación de checkout, commit y cambios locales antes de compilar.
- [x] Compilación incremental del objetivo `bootcd` y comprobación de que exista la ISO.
- [x] Lanzador opcional de QEMU con memoria configurable.
- [ ] Clonar ReactOS: requiere conexión a GitHub.
- [ ] Compilar ReactOS: requiere fuente, dependencias y entorno compatible.
- [ ] Arrancar ISO en QEMU: requiere ISO y QEMU.
- [ ] Aplicar identidad MosaicOS y validar aplicaciones/drivers.

No se afirma que exista una ISO compilada ni que haya compatibilidad de aplicaciones probada.
