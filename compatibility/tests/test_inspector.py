#!/usr/bin/env python3
"""Self-contained synthetic PE/MSI tests; no Windows binaries required."""
import importlib.util, struct, tempfile, unittest
from pathlib import Path
TOOL=Path(__file__).parents[1]/"tools"/"inspect-windows-file.py"
spec=importlib.util.spec_from_file_location("mosaic_inspector",TOOL)
mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)

def minimal_pe(bits=32):
    opt_size=224 if bits==32 else 240
    b=bytearray(0x400)
    b[:2]=b"MZ"; struct.pack_into("<I",b,0x3c,0x80); b[0x80:0x84]=b"PE\0\0"
    struct.pack_into("<HHIIIHH",b,0x84,0x14c if bits==32 else 0x8664,1,0,0,0,opt_size,0x0102)
    opt=0x98; struct.pack_into("<H",b,opt,0x10b if bits==32 else 0x20b)
    struct.pack_into("<H",b,opt+68,3)
    # One .rdata section: RVA 0x1000 -> raw offset 0x200
    sec=opt+opt_size; b[sec:sec+8]=b".rdata\0\0"
    struct.pack_into("<IIII",b,sec+8,0x200,0x1000,0x200,0x200)
    # Import directory at RVA 0x1000, descriptor name at RVA 0x1040
    dd=96 if bits==32 else 112; struct.pack_into("<I",b,opt+(92 if bits==32 else 108),16); struct.pack_into("<II",b,opt+dd+8,0x1000,40)
    struct.pack_into("<IIIII",b,0x200,0,0,0,0x1040,0)
    b[0x240:0x24d]=b"KERNEL32.dll\0"
    return bytes(b)
class InspectorTests(unittest.TestCase):
    def test_pe32_import(self):
        r=mod.inspect_pe(minimal_pe(32)); self.assertEqual(r["bits"],32); self.assertIn("kernel32.dll",r["dll_imports"])
    def test_pe64(self): self.assertEqual(mod.inspect_pe(minimal_pe(64))["bits"],64)
    def test_msi_signature(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/"sample.msi"; p.write_bytes(mod.MSI_CFB+b"\0"*32)
            self.assertEqual(mod.inspect(p)["format"],"OLE_compound_file")
    def test_unknown(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/"x.bin"; p.write_bytes(b"not a PE")
            self.assertEqual(mod.inspect(p)["format"],"unknown")
    def test_truncated_pe(self):
        with self.assertRaises(mod.FormatError): mod.inspect_pe(b"MZ"+b"\0"*10)
if __name__=="__main__": unittest.main(verbosity=2)
