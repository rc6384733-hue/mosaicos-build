#!/usr/bin/env python3
"""Validación estática básica del binario ELF32 de Mosaic Core."""
from pathlib import Path
import struct
import sys

image = Path(sys.argv[1] if len(sys.argv) > 1 else "mosaicos.bin").read_bytes()
root = Path(__file__).resolve().parents[1]
source = (root / "kernel.c").read_text(encoding="utf-8")

if len(image) < 52:
    raise SystemExit("FAIL: imagen demasiado pequeña para ELF32")
if image[:4] != b"\x7fELF":
    raise SystemExit("FAIL: falta cabecera ELF")
if image[4] != 1 or image[5] != 1:
    raise SystemExit("FAIL: se esperaba ELF de 32 bits little-endian")
e_machine = struct.unpack_from("<H", image, 18)[0]
if e_machine != 3:
    raise SystemExit(f"FAIL: arquitectura ELF inesperada (e_machine={e_machine})")
if b"\x02\xb0\xad\x1b" not in image[:8192]:
    raise SystemExit("FAIL: no se encuentra la magia Multiboot en los primeros 8 KiB")
if b"MosaicOS 0.31" not in image:
    raise SystemExit("FAIL: falta la identificación de Mosaic Core 0.31")
if "--- texto de la pagina ---" not in source:
    raise SystemExit("FAIL: falta el marcador esperado del visor HTTPS")
print(f"OK Mosaic Core ELF32/Multiboot; {len(image)} bytes")
