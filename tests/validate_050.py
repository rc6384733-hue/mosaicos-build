#!/usr/bin/env python3
"""Validación estática de MosaicOS 0.50 Aero (i386, Debian 12 Bookworm).

No construye ni arranca la ISO: comprueba que la receta, los recursos y las
integraciones de escritorio son coherentes antes de compilar.
"""
from pathlib import Path
import configparser
import re
import struct
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
DESKTOP = ROOT / "desktop"
CHROOT = DESKTOP / "config" / "includes.chroot"
errors = []


def check(condition, message):
    if not condition:
        errors.append(message)


def read(path):
    return path.read_text(encoding="utf-8", errors="replace")


# ---------------------------------------------------------------- versión
version = read(ROOT / "VERSION").strip()
check(version == "0.50", f"VERSION debe ser 0.50; es {version!r}")

# ---------------------------------------------------------------- receta
build = read(DESKTOP / "build.sh")
for needle, message in [
    ("--architectures i386", "La ISO debe compilarse para i386"),
    ("--distribution \"$CODENAME\"", "Falta la distribución de live-build"),
    ("CODENAME=bookworm", "La base debe ser Debian 12 Bookworm"),
    ("--bootloaders syslinux", "Falta el cargador BIOS syslinux/isolinux"),
    ("--debian-installer live", "Falta el instalador Debian live"),
    ("MOSAICOS_050_I386", "El volumen de la ISO no es 0.50"),
    ("MosaicOS-$VERSION-i386.iso", "El nombre de salida no está versionado"),
    ("MOSAIC_MIN_FREE_GIB", "Falta el control de espacio libre"),
    ("sha256sum", "Falta la suma SHA-256 de la ISO"),
]:
    check(needle in build, message)
check("wine64" not in build, "La receta no debe incluir wine64")
check("win7" not in build.lower(), "El escritorio web no debe formar parte de la ISO")
check("--architectures amd64" not in build, "0.50 es una edición exclusiva i386")

# ---------------------------------------------------------------- paquetes
profile = read(DESKTOP / "config" / "package-lists" / "mosaic-desktop-i386.packages")
package_lines = [
    line.strip()
    for line in profile.splitlines()
    if line.strip() and not line.strip().startswith("#")
]
for required in [
    "linux-image-686-pae",
    "lxqt-core",
    "lxqt-panel",
    "openbox",
    "lightdm",
    "pcmanfm-qt",
    "picom",
    "featherpad",
    "network-manager",
    "firefox-esr",
    "thunderbird",
    "libreoffice-writer",
    "vlc",
    "gimp",
    "wine",
    "wine32",
    "wine-binfmt",
    "winetricks",
    "debian-installer-launcher",
    "gparted",
    "synaptic",
    "yad",
    "firmware-iwlwifi",
    "fonts-wine",
    "zram-tools",
    "earlyoom",
]:
    check(required in package_lines, f"Falta el paquete obligatorio: {required}")
for forbidden in ["wine64", "virtualbox-guest-utils", "virtualbox-guest-x11"]:
    check(forbidden not in package_lines, f"Paquete no válido en i386: {forbidden}")
duplicates = {p for p in package_lines if package_lines.count(p) > 1}
check(not duplicates, f"Paquetes duplicados: {', '.join(sorted(duplicates))}")

# ---------------------------------------------------------------- recursos
bg = CHROOT / "usr" / "share" / "backgrounds" / "mosaicos"
for name in ("mosaic-7.png", "mosaic-7-login.png"):
    path = bg / name
    check(path.is_file() and path.stat().st_size > 10_000, f"Falta el fondo {name}")
    if path.is_file():
        check(path.read_bytes()[:8] == b"\x89PNG\r\n\x1a\n", f"{name} no es un PNG válido")

icon_root = CHROOT / "usr" / "share" / "icons" / "MosaicAero"
check((icon_root / "index.theme").is_file(), "Falta index.theme de MosaicAero")
check("Name=MosaicAero" in read(icon_root / "index.theme"), "El tema de iconos no se llama MosaicAero")
for size in (16, 22, 24, 32, 48, 64, 128, 256):
    icon = icon_root / f"{size}x{size}" / "apps" / "start-here.png"
    check(icon.is_file() and icon.stat().st_size > 0, f"Falta el icono de inicio {size}px")

theme = CHROOT / "usr" / "share" / "themes" / "MosaicAero" / "openbox-3"
themerc = read(theme / "themerc")
for needle in ("window.active.title.bg", "window.active.button.unpressed.image.color", "menu.items.bg"):
    check(needle in themerc, f"themerc incompleto: {needle}")
for button in (
    "close", "close_hover", "close_pressed",
    "max", "max_hover", "max_pressed",
    "max_toggled", "max_toggled_hover", "max_toggled_pressed",
    "iconify", "iconify_hover", "iconify_pressed",
):
    xbm = theme / f"{button}.xbm"
    check(xbm.is_file(), f"Falta la máscara Openbox {button}.xbm")
    if xbm.is_file():
        text = read(xbm)
        check("_width 16" in text and "_height 16" in text, f"Máscara {button}.xbm con tamaño incorrecto")

# ---------------------------------------------------------------- escritorio
panel = CHROOT / "etc" / "skel" / ".config" / "lxqt" / "panel.conf"
panel_text = read(panel)
check("[General]" in panel_text and "panels=panel1" in panel_text,
      "El panel no declara su lista de paneles en [General]")
check("[panel1]" in panel_text and "position=Bottom" in panel_text, "El panel no está abajo")
check("lockPanel=false" in panel_text, "La clave lockPanel no es válida para LXQt")
for plugin in ("mainmenu", "taskbar", "statusnotifier", "worldclock"):
    check(f"plugins=" in panel_text and plugin in panel_text, f"El panel no incluye {plugin}")
check("icon=start-here" in panel_text, "El menú de inicio no usa el orbe de MosaicOS")

lxqt = read(CHROOT / "etc" / "skel" / ".config" / "lxqt" / "lxqt.conf")
check("icon_theme=MosaicAero" in lxqt, "LXQt no usa el tema de iconos MosaicAero")

rc_xml = CHROOT / "etc" / "skel" / ".config" / "openbox" / "rc.xml"
try:
    rc = ET.parse(rc_xml)
    ns = {"o": "http://openbox.org/3.4/rc"}
    check(rc.findtext(".//o:theme/o:name", namespaces=ns) == "MosaicAero",
          "Openbox no usa el tema MosaicAero")
    title_layout = rc.findtext(".//o:theme/o:titleLayout", namespaces=ns) or ""
    check("C" in title_layout and "M" in title_layout, "La barra de título no tiene cerrar/maximizar")
    keys = [k.get("key") for k in rc.findall(".//o:keybind", ns)]
    for combo in ("W-e", "W-t", "W-d", "A-F4"):
        check(combo in keys, f"Falta el atajo {combo}")
except (ET.ParseError, OSError) as exc:
    errors.append(f"rc.xml inválido: {exc}")

# Entradas .desktop del sistema y del escritorio.
def parse_desktop(path):
    parser = configparser.ConfigParser(interpolation=None, strict=False)
    parser.optionxform = str
    parser.read(path, encoding="utf-8")
    return parser


for path in sorted((CHROOT / "usr" / "share" / "applications").glob("*.desktop")):
    parser = parse_desktop(path)
    check(parser.has_section("Desktop Entry"), f"Entrada sin [Desktop Entry]: {path.name}")
    if parser.has_section("Desktop Entry"):
        entry = parser["Desktop Entry"]
        for key in ("Type", "Name", "Exec"):
            check(key in entry, f"{path.name} sin clave {key}")

escritorio = CHROOT / "etc" / "skel" / "Escritorio"
shortcuts = sorted(escritorio.glob("*.desktop")) if escritorio.is_dir() else []
check(len(shortcuts) >= 5, "Faltan accesos directos en el Escritorio español")
check(not (CHROOT / "etc" / "skel" / "Desktop").exists(), "No debe existir la carpeta Desktop en inglés")
shortcut_names = {p.name for p in shortcuts}
for expected in (
    "Instalar MosaicOS.desktop",
    "Aplicaciones Windows.desktop",
    "Mosaic Store.desktop",
    "Centro Mosaic.desktop",
    "Configuración Mosaic.desktop",
):
    check(expected in shortcut_names, f"Falta el acceso directo: {expected}")

autostart = CHROOT / "etc" / "xdg" / "autostart"
for expected in (
    "mosaic-wine-setup.desktop",
    "mosaic-first-run.desktop",
    "picom.desktop",
    "nm-tray.desktop",
    "mosaic-startup-sound.desktop",
    "xdg-user-dirs.desktop",
):
    check((autostart / expected).is_file(), f"Falta el autostart {expected}")

# ---------------------------------------------------------------- herramientas
bin_dir = CHROOT / "usr" / "local" / "bin"
for script in (
    "mosaic-run-exe",
    "mosaic-wine-setup",
    "mosaic-windows",
    "mosaic-install-disk",
    "mosaic-welcome",
    "mosaic-welcome-gui",
    "mosaic-first-run",
    "mosaic-startup-sound",
    "mosaic-center",
    "mosaic-settings",
    "mosaic-health",
    "mosaic-report",
):
    path = bin_dir / script
    check(path.is_file(), f"Falta la herramienta {script}")
    if path.is_file():
        text = read(path)
        check(text.startswith("#!/bin/sh"), f"{script} no tiene shebang /bin/sh")

run_exe = read(bin_dir / "mosaic-run-exe")
check("PE32+" in run_exe and "64" in run_exe, "mosaic-run-exe no rechaza binarios de 64 bits")
check("WINEARCH=win32" in run_exe, "mosaic-run-exe no fuerza un prefijo Win32")
wine_setup = read(bin_dir / "mosaic-wine-setup")
check("Version /d win7" in wine_setup, "Wine no se configura como Windows 7")
check("WINEARCH=win32" in wine_setup, "El prefijo Wine no es Win32")
install = read(bin_dir / "mosaic-install-disk")
check("debian-installer-launcher" in install, "El instalador no usa debian-installer-launcher")

mime = read(CHROOT / "etc" / "skel" / ".config" / "mimeapps.list")
check("mosaic-run-exe.desktop" in mime, "Los .exe no están asociados a Wine")
check((CHROOT / "usr" / "share" / "applications" / "mosaic-run-exe.desktop").is_file(),
      "Falta la entrada de escritorio mosaic-run-exe para los .exe")
check((CHROOT / "etc" / "skel" / ".local" / "share" / "file-manager" / "actions" / "wine.desktop").is_file(),
      "Falta la acción de PCManFM-Qt para Wine")

# ---------------------------------------------------------------- audio y hook
wav = CHROOT / "usr" / "share" / "sounds" / "mosaic" / "startup.wav"
check(wav.is_file(), "Falta el sonido de inicio startup.wav")
if wav.is_file():
    data = wav.read_bytes()
    check(data[:4] == b"RIFF" and data[8:12] == b"WAVE", "startup.wav no es un WAV válido")
    check(len(data) > 100_000, "startup.wav es demasiado corto")
    if len(data) >= 44 and data[:4] == b"RIFF":
        channels, rate = struct.unpack("<HI", data[22:28])
        check(channels == 2 and rate == 44100, "startup.wav no es estéreo a 44,1 kHz")

hook = DESKTOP / "config" / "hooks" / "live" / "0100-mosaic-configuration.hook.chroot"
check(hook.is_file(), "Falta el hook de configuración del chroot")
if hook.is_file():
    hook_text = read(hook)
    for needle in ("locale-gen", "useradd", "usermod", "chmod", "systemctl enable"):
        check(needle in hook_text, f"El hook no contiene {needle}")

check((CHROOT / "etc" / "xdg" / "picom.conf").is_file(), "Falta la configuración global de picom")
check((CHROOT / "etc" / "lightdm" / "lightdm.conf").is_file(), "Falta la configuración de LightDM")
check((CHROOT / "etc" / "default" / "keyboard").is_file(), "Falta la configuración de teclado")
check("XKBLAYOUT=\"es\"" in read(CHROOT / "etc" / "default" / "keyboard"), "El teclado no es español")

# ---------------------------------------------------------------- compilación
for rel in (
    "desktop/docker/Dockerfile",
    "desktop/docker/entrypoint.sh",
    "desktop/docker/build-with-docker.sh",
    "desktop/docker/build-with-docker.ps1",
    ".github/workflows/build-iso.yml",
    "desktop/prepare-build-host.sh",
    "desktop/verify-iso.sh",
):
    check((ROOT / rel).is_file(), f"Falta el archivo de compilación: {rel}")

dockerfile = read(DESKTOP / "docker" / "Dockerfile")
check("FROM debian:bookworm" in dockerfile, "El contenedor no usa Debian Bookworm")
check("live-build" in dockerfile and "xorriso" in dockerfile, "El contenedor no instala live-build/xorriso")

workflow = read(ROOT / ".github" / "workflows" / "build-iso.yml")
check("mosaicos-builder" in workflow and "validate_050.py" in workflow,
      "El workflow no valida ni usa el contenedor de compilación")

# ---------------------------------------------------------------- resultado
if errors:
    print("FALLOS:")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print("OK: MosaicOS 0.50 Aero i386 — receta, escritorio, Wine32, recursos y CI coherentes.")
