"""Run inside GDB to inspect a failed standard C assertion."""

import json
import os
from pathlib import Path
import re
import shlex

import gdb


IDENTIFIER = r"[A-Za-z_]\w*"
INDEX = rf"(?:{IDENTIFIER}|-?\d+)"
REFERENCE = rf"{IDENTIFIER}(?:(?:->|\.){IDENTIFIER}|\[\s*{INDEX}\s*\])*"
OPERAND = rf"(?:{REFERENCE}|-?\d+|'(?:\\.|[^'\\])')"
outcome = {}


def exited(event):
  outcome.update(kind="exit", code=getattr(event, "exit_code", 1))


def stopped(event):
  outcome.update(kind="signal", signal=getattr(event, "stop_signal", "unknown"))


def format_value(value):
  if value.type.strip_typedefs().code in (gdb.TYPE_CODE_INT, gdb.TYPE_CODE_CHAR):
    number = int(value)
    if value.type.sizeof == 1 and 32 <= number <= 126:
      return f"{number} ({chr(number)!r})"
    return str(number)
  return str(value)


def inspect_assertion(output):
  match = re.search(r": (.+):(\d+): (.+): Assertion `(.*)' failed\.", output)
  if not match:
    return
  filename, line, function, expression = match.groups()
  outcome.update(kind="assertion", file=filename, line=int(line),
                 function=function, expression=expression)

  frame = gdb.newest_frame()
  while frame:
    location = frame.find_sal()
    if (location.symtab and location.line == int(line) and
        Path(location.symtab.fullname()).resolve() == Path(filename).resolve()):
      break
    frame = frame.older()
  if frame is None:
    return
  frame.select()

  # Look for a named table row or case pointer referenced by the assertion.
  candidates = re.findall(rf"{IDENTIFIER}\[\s*{INDEX}\s*\]|{IDENTIFIER}(?=->)", expression)
  for candidate in candidates:
    try:
      value = gdb.parse_and_eval(candidate)
      if value.type.strip_typedefs().code == gdb.TYPE_CODE_PTR:
        value = value.dereference()
      outcome["case"] = value["name"].string()
      break
    except (gdb.error, UnicodeError):
      continue

  # Only inspect references and literals; never rerun calls or assignments.
  comparison = re.fullmatch(rf"\s*({OPERAND})\s*(==|!=|<=|>=|<|>)\s*({OPERAND})\s*", expression)
  if comparison:
    left, operator, right = comparison.groups()
    try:
      outcome.update(left=left, right=right, operator=operator,
                     actual=format_value(gdb.parse_and_eval(left)),
                     expected=format_value(gdb.parse_and_eval(right)))
    except gdb.error:
      pass


gdb.events.exited.connect(exited)
gdb.events.stop.connect(stopped)
try:
  gdb.execute("set pagination off")
  gdb.execute("set confirm off")
  gdb.execute("set disable-randomization off")
  gdb.execute("set debuginfod enabled off")
  gdb.execute("run > " + shlex.quote(os.environ["C_TEST_OUTPUT"]) + " 2>&1")
  if outcome.get("kind") == "signal":
    inspect_assertion(Path(os.environ["C_TEST_OUTPUT"]).read_text(errors="replace"))
except gdb.error as error:
  outcome.update(kind="error", message=str(error))
finally:
  Path(os.environ["C_TEST_REPORT"]).write_text(json.dumps(outcome))
