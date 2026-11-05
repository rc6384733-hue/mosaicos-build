#!/usr/bin/env python3
"""Inspect Windows PE files without executing them; standard library only."""
from __future__ import annotations
import argparse, json, struct, sys
from pathlib import Path

MACHINES = {0x014C: "x86", 0x8664: "x86_64", 0x01C0: "ARM", 0x01C4: "ARMv7", 0xAA64: "ARM64"}
SUBSYSTEMS = {1: "native", 2: "windows_gui", 3: "windows_console", 9: "windows_ce_gui", 10: "efi_application"}
MSI_CFB = bytes.fromhex("D0CF11E0A1B11AE1")

class FormatError(Exception): pass

def u16(b: bytes, o: int) -> int:
    if o < 0 or o + 2 > len(b): raise FormatError("unexpected end of file")
    return struct.unpack_from("<H", b, o)[0]

def u32(b: bytes, o: int) -> int:
    if o < 0 or o + 4 > len(b): raise FormatError("unexpected end of file")
    return struct.unpack_from("<I", b, o)[0]

def cstring(b: bytes, o: int, limit: int = 512) -> str:
    if o < 0 or o >= len(b): raise FormatError("string offset outside file")
    end = b.find(b"\0", o, min(len(b), o + limit))
    if end < 0: raise FormatError("unterminated string")
    return b[o:end].decode("ascii", "replace")

def inspect_pe(data: bytes) -> dict:
    if len(data) < 64 or data[:2] != b"MZ": raise FormatError("missing DOS MZ header")
    pe = u32(data, 0x3C)
    if data[pe:pe+4] != b"PE\0\0": raise FormatError("missing PE signature")
    coff = pe + 4
    machine, section_count = u16(data, coff), u16(data, coff + 2)
    optional_size, characteristics = u16(data, coff + 16), u16(data, coff + 18)
    opt = coff + 20
    if opt + optional_size > len(data): raise FormatError("truncated optional header")
    magic = u16(data, opt)
    if magic == 0x10B:
        bits, dd_off, num_dd_off, thunk_width, ordinal_mask = 32, 96, 92, 4, 0x80000000
    elif magic == 0x20B:
        bits, dd_off, num_dd_off, thunk_width, ordinal_mask = 64, 112, 108, 8, 0x8000000000000000
    else: raise FormatError(f"unsupported optional-header magic 0x{magic:04x}")
    subsystem = u16(data, opt + 68)
    sections, sec_off = [], opt + optional_size
    if sec_off + section_count * 40 > len(data): raise FormatError("truncated section table")
    for i in range(section_count):
        o = sec_off + i * 40
        name = data[o:o+8].split(b"\0",1)[0].decode("ascii","replace")
        virtual_size, virtual_address, raw_size, raw_ptr = struct.unpack_from("<IIII", data, o+8)
        sections.append((name, virtual_address, max(virtual_size, raw_size), raw_ptr, raw_size))
    def rva_to_offset(rva: int) -> int:
        if rva < opt + optional_size: return rva
        for _, va, span, raw, raw_size in sections:
            if va <= rva < va + span:
                delta = rva - va
                if delta >= raw_size: raise FormatError("RVA points to uninitialized section data")
                return raw + delta
        raise FormatError(f"RVA 0x{rva:x} is not mapped by a section")
    imports, import_error = [], None
    number_dirs = u32(data, opt + num_dd_off)
    if number_dirs > 1 and dd_off + 16 <= optional_size:
        import_rva, import_size = struct.unpack_from("<II", data, opt + dd_off + 8)
        if import_rva:
            try:
                desc = rva_to_offset(import_rva)
                for _ in range(4096):
                    if desc + 20 > len(data): raise FormatError("truncated import descriptor")
                    oft, timestamp, chain, name_rva, first = struct.unpack_from("<IIIII", data, desc)
                    if not (oft or timestamp or chain or name_rva or first): break
                    dll = cstring(data, rva_to_offset(name_rva)).lower()
                    imports.append(dll)
                    desc += 20
                else: raise FormatError("import descriptor limit exceeded")
            except FormatError as exc: import_error = str(exc)
    return {"format":"PE", "architecture":MACHINES.get(machine, f"unknown_0x{machine:04x}"),
            "bits":bits, "subsystem":SUBSYSTEMS.get(subsystem, f"unknown_{subsystem}"),
            "sections":section_count, "characteristics":f"0x{characteristics:04x}",
            "dll_imports":sorted(set(imports)), "import_table_error":import_error,
            "compatibility":"unknown_requires_execution_test"}

def inspect(path: Path) -> dict:
    data = path.read_bytes()
    base = {"file":str(path), "size_bytes":len(data)}
    if data.startswith(b"MZ"):
        try: return base | inspect_pe(data)
        except FormatError as exc: return base | {"format":"invalid_or_truncated_PE", "error":str(exc)}
    if data.startswith(MSI_CFB):
        return base | {"format":"OLE_compound_file", "likely_type":"MSI_or_other_CFB", "compatibility":"unknown_requires_Windows_Installer_test"}
    if data.startswith(b"PK\x03\x04"):
        return base | {"format":"ZIP_archive", "possible_types":["MSIX_or_unrelated_zip"], "compatibility":"unknown"}
    return base | {"format":"unknown", "compatibility":"unknown"}

def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("file",type=Path)
    p.add_argument("--json",action="store_true",help="emit machine-readable JSON")
    a=p.parse_args()
    try: result=inspect(a.file)
    except OSError as e: print(f"ERROR: {e}",file=sys.stderr); return 2
    if a.json: print(json.dumps(result,indent=2,sort_keys=True))
    else:
        for k,v in result.items(): print(f"{k.replace('_',' ').title()}: {', '.join(v) if isinstance(v,list) else v}")
        print("Nota: inspeccionar un archivo no demuestra que MosaicOS pueda ejecutarlo.")
    return 0 if result.get("format") not in ("unknown","invalid_or_truncated_PE") else 1
if __name__=="__main__": raise SystemExit(main())
