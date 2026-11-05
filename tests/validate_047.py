#!/usr/bin/env python3
"""Alias de compatibilidad: los tests 0.47 quedaron reemplazados por 0.50."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
sys.exit(subprocess.run([sys.executable, str(root / "tests/validate_050.py")]).returncode)
