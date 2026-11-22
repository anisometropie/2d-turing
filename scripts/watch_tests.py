#!/usr/bin/env python3
"""Run C tests on startup and after source files are saved."""

import os
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
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


def execute_test(binary):
  with tempfile.TemporaryDirectory(prefix="c-test-report-") as directory:
    output = Path(directory) / "output.txt"
    report = Path(directory) / "report.json"
    debugger = subprocess.run(
      ["gdb", "--quiet", "--nx", "--batch", "-x", str(ROOT / "scripts/assertion_report.py"),
       "--args", str(ROOT / binary)], cwd=ROOT,
      env={**os.environ, "C_TEST_OUTPUT": str(output), "C_TEST_REPORT": str(report)},
      stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if not report.exists():
      return {"kind": "error", "message": debugger.stdout}, ""
    result = json.loads(report.read_text())
    return result, output.read_text(errors="replace") if output.exists() else ""


def print_failure(result, output):
  if result.get("kind") == "assertion":
    case = f" / {result['case']}" if "case" in result else ""
    print(f"  {result['function']}{case}", flush=True)
    print(f"  {result['file']}:{result['line']}", flush=True)
    if "actual" in result and result["operator"] == "==":
      print(f"  {result['left']}: expected {result['expected']}, got {result['actual']}", flush=True)
    else:
      print(f"  Assertion failed: {result['expression']}", flush=True)
      if "actual" in result:
        print(f"  {result['left']} = {result['actual']}; {result['right']} = {result['expected']}", flush=True)
  else:
    if output:
      print(output, end="", flush=True)
    if result.get("kind") == "signal":
      print(f"  Stopped by {result['signal']}", flush=True)
    elif result.get("kind") == "error":
      print(f"  Debugger error: {result['message']}", flush=True)
    else:
      print(f"  Exit code: {result.get('code', 'unknown')}", flush=True)


def run_tests():
  binaries = [f"build/tests/{path.stem}" for path in sorted((ROOT / "tests").glob("*_test.c"))]
  if not binaries:
    print("No tests found.", flush=True)
    return 0
  if shutil.which("gdb") is None:
    print("Test reporting requires GDB. Install it with: sudo apt install gdb", flush=True)
    return 1

  build = subprocess.run(
    ["make", "--silent", "--no-print-directory", *binaries], cwd=ROOT,
    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
  if build.returncode != 0:
    print(build.stdout, end="", flush=True)
    return 1

  failed = False
  for binary in binaries:
    result, output = execute_test(binary)
    name = Path(binary).stem.removesuffix("_test")
    passed = result.get("kind") == "exit" and result.get("code") == 0
    status = "passed" if passed else "FAILED"
    print(f"{name}: {status}", flush=True)
    if not passed:
      failed = True
      print_failure(result, output)
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
