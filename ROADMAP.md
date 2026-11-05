# Hoja de ruta MosaicOS

1. **0.47 — construcción reproducible:** perfiles separados, workspaces aislados y validación estática. Completado.
2. **0.48 — primera compilación:** producir ISO amd64 e i386 en un host preparado, conservar logs y hashes. Receta completada; compilación real pendiente.
3. **0.49 — validación virtual:** arrancar en QEMU, corregir fallos de sesión, red, almacenamiento y apagado. Pendiente.
4. **0.50 — escritorio real de 32 bits:** sustituir el escritorio web por LXQt/Openbox con tema Aero, Wine32 y aplicaciones Linux nativas. Receta completada; ISO pendiente de compilar.
5. **0.51 — primera ISO 0.50:** compilar en Debian/Docker/GitHub Actions, publicar hash y log, arrancar en QEMU BIOS.
6. **0.52 — sesión live estable:** corregir panel, tema, red, audio, apagado e instalador; probar en hardware real de 32 bits.
7. **0.53 — instalación en disco:** llevar el tema y las herramientas Mosaic al sistema instalado; evaluar un instalador propio.
8. **0.54 — compatibilidad Windows:** matriz de aplicaciones Win32, recetas de `winetricks`, aislamiento y documentación por aplicación.
9. **0.55 — rendimiento:** medir RAM/CPU/arranque en 1–2 GB, ajustar servicios y zram.
10. **Siguiente línea:** Mosaic Experience, gestor de compatibilidad y seguridad reforzada con pruebas reproducibles.

No se declarará una función como terminada solo por existir una receta o un botón. Debe funcionar y tener una prueba registrada.
