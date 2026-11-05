# Inicio rápido — MosaicOS 0.50 Aero (i386)

## 1. Elegir cómo compilar

| Entorno | Camino recomendado |
|---|---|
| Debian 12/13 | `sudo sh ./desktop/prepare-build-host.sh` y `sudo sh ./desktop/build.sh i386` |
| Windows/macOS con Docker Desktop | `powershell -ExecutionPolicy Bypass -File .\desktop\docker\build-with-docker.ps1` |
| Linux/macOS con Docker | `sh ./desktop/docker/build-with-docker.sh` |
| GitHub | Ejecutar el workflow «Build MosaicOS ISO (i386)» |

La ISO tarda bastante y necesita conexión a Internet y unos 20–30 GiB libres.

## 2. Probar en QEMU

```sh
qemu-system-i386 -m 2048 -cdrom desktop/dist/MosaicOS-0.50-i386.iso -boot d
```

Sin instalar nada:

1. Espera a que aparezca el escritorio.
2. Comprueba el panel, el menú de inicio y la red.
3. Abre Firefox, un archivo de texto y el terminal.
4. Abre «Aplicaciones Windows» y prueba un `.exe` de 32 bits.
5. Apaga y vuelve a arrancar.

No uses el instalador sobre un disco con datos sin copia de seguridad.

## 3. Usar el escritorio

- **Menú de inicio:** orbe azul en el extremo izquierdo del panel.
- **Archivos:** `Super+E` o el icono del panel.
- **Terminal:** `Super+T`.
- **Ejecutar:** `Super+R`.
- **Configuración:** `Super+P` o «Configuración Mosaic».
- **Aplicaciones Windows:** `Super+W` o el acceso directo del escritorio.
- **Captura de pantalla:** tecla `Impr Pant`.

## 4. Instalar aplicaciones Linux

- **Mosaic Store:** catálogo con búsqueda e instalación confirmada.
- **Synaptic:** gestor APT completo.
- **GDebi:** instalación de archivos `.deb`.
- **Terminal:** `sudo apt install paquete`.

## 5. Aplicaciones Windows

Wine32 se prepara en el primer inicio. Para abrir un programa:

1. Abre «Aplicaciones Windows».
2. Pulsa «Abrir un .exe» y selecciona el archivo.
3. O haz doble clic sobre un `.exe` en el explorador de archivos.

Solo funciona software **Windows de 32 bits**. Los programas de 64 bits se rechazan con un aviso. La compatibilidad varía: algunas aplicaciones necesitan `winetricks` o no funcionan.

## 6. Instalar en disco

- Desde el escritorio: «Instalar MosaicOS».
- Desde el menú de arranque: entrada «Instalar».

El instalador es el de Debian. La sesión live es la experiencia MosaicOS completa; el sistema instalado es una base Debian y puede requerir ajustes adicionales.

## 7. Limitaciones que debes conocer

- No hay ISO precompilada en este ZIP.
- La ISO es de 32 bits y arranca por BIOS, no por UEFI de 32 bits.
- No ejecuta aplicaciones Windows de 64 bits.
- La compatibilidad con Wine no es universal.
- El rendimiento no está medido en equipos con 1 GB de RAM.

Consulta `INSTALL.md` para el procedimiento detallado y `BUILD_REPORT-0.50.txt` para saber qué se validó realmente.
