#!/usr/bin/env python3
"""Run C tests on startup and after source files are saved."""

import os
from pathlib import Path
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[1]
IGNORED = {".git", ".agents", ".aws", ".codex", "build", "__pycache__"}


def snapshot():
  files = {}
  for directory, folders, names in os.walk(ROOT):
    folders[:] = [name for name in folders if name not in IGNORED]
    for name in names:
      path = Path(directory) / name
      if path.suffix not in {".c", ".h", ".mk"} and name != "Makefile":
        continue
      try:
        stat = path.stat()
        files[path] = (stat.st_mtime_ns, stat.st_size, stat.st_ino)
      except FileNotFoundError:
        pass  # Editors may replace a file while saving it.
  return files


def run_tests():
  binaries = [f"build/tests/{path.stem}" for path in sorted((ROOT / "tests").glob("*_test.c"))]
  if not binaries:
    print("No tests found.", flush=True)
    return 0

  build = subprocess.run(
    ["make", "--silent", "--no-print-directory", *binaries], cwd=ROOT,
    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
  if build.returncode != 0:
    print(build.stdout, end="", flush=True)
    return 1

  failed = False
  for binary in binaries:
    result = subprocess.run(
      [str(ROOT / binary)], cwd=ROOT,
      stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    name = Path(binary).stem.removesuffix("_test")
    status = "passed" if result.returncode == 0 else "FAILED"
    print(f"{name}: {status}", flush=True)
    if result.returncode != 0:
      failed = True
      print(result.stdout, end="", flush=True)
  return int(failed)


def main():
  previous = snapshot()
  run_tests()
  while True:
    time.sleep(0.25)
    current = snapshot()
    if current != previous:
      time.sleep(0.15)  # Allow an editor's save operation to finish.
      previous = snapshot()
      run_tests()


if __name__ == "__main__":
  try:
    if "--once" in sys.argv:
      sys.exit(run_tests())
    main()
  except KeyboardInterrupt:
    pass
