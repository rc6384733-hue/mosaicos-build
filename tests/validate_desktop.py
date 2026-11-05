#!/usr/bin/env python3
"""Alias histórico; ejecuta la validación del escritorio vigente (0.50)."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
sys.exit(subprocess.run([sys.executable, str(root / "tests/validate_050.py")]).returncode)
