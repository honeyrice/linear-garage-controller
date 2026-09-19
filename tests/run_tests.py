"""Offline checks only: no serial, network, HA or door operations."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)

with tempfile.TemporaryDirectory(prefix='linear-garage-tests-') as tmp:
    for name in ('gate_controller_v2_test', 'position_tracker_test'):
        exe = str(Path(tmp) / name)
        run(os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-pedantic',
            *shlex.split(os.environ.get('CXXFLAGS', '')),
            str(ROOT / 'tests' / (name + '.cpp')), '-o', exe)
        run(exe, *([str(ROOT / 'tests/field-counts.txt')] if name == 'position_tracker_test' else []))
run(sys.executable, 'home-assistant/test_statistics.py')
run(sys.executable, 'home-assistant/test_cover_migration.py')
print('All offline checks passed.')
