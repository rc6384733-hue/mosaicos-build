# Perfiles de hardware — objetivos, no garantías

| Perfil | RAM | Arquitectura | Uso razonable esperado |
|---|---:|---|---|
| Legacy | 1 GB | x86 32-bit cuando la base y el navegador aún lo soporten | Escritorio, archivos y tareas locales sencillas; web moderna muy limitada |
| Classic | 2 GB | x86 32/64-bit | Navegación limitada y ofimática ligera, según CPU y navegador disponible |
| Standard | 4 GB | x86-64 recomendado | Uso cotidiano más viable |
| Plus | 8 GB o más | x86-64 | Multitarea, desarrollo y compatibilidad adicional |

## Principios
- Detectar RAM, CPU, arquitectura, extensiones, GPU, almacenamiento, red y firmware.
- No cargar IA generativa local automáticamente en equipos de 1 GB.
- No instalar Wine/Proton/DXVK por defecto en Legacy; ofrecer instalación bajo demanda.
- Proton es principalmente para juegos vía Steam; no equivale a compatibilidad universal de escritorio.
- DXVK requiere soporte Vulkan en GPU/controlador; GPU antiguas pueden no tenerlo.
- Usar zram/swap solo tras medir; swap no sustituye RAM y puede ser lento en HDD.
- No retirar actualizaciones, aislamiento ni protecciones para reducir consumo.
- Publicar resultados de pruebas por equipo, no promesas genéricas.
