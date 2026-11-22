# 2d-turing

A small SDL2 program that draws a square in a window.

Install SDL2 development headers and `pkg-config` first (on Debian or Ubuntu:
`sudo apt install libsdl2-dev pkg-config`). Then build with `make` and run with
`./2d-turing`. Resize the window to keep the square centered. Press Escape or
use the close button to exit. Use `make clean` to remove the executable.

## Unit tests

Run `make test` once, or `make watch-test` to rerun tests whenever you save a C
file, header, or Makefile. The watcher needs Python 3 and GDB (`sudo apt install gdb`).
It runs immediately and keeps watching after failures. Press Ctrl+C to stop it.

In VS Code, run the **Watch unit tests** task. It is also configured to start when
the folder opens; allow automatic tasks for this folder if VS Code prompts you.

Add a file named `tests/<module>_test.c` with an `int main(void)` function and
checks using `<assert.h>`. It automatically builds with `<module>.c` and runs as
part of `make test`. Each test file runs as its own executable. A failed assertion
reports its file and line and fails the run.
Tests build with debug information and run under GDB. Keep using ordinary
`assert(...)`: the runner reports the test function, the active table row's
`name` field, and values from simple comparisons. Other expressions retain their
original assertion message. Execution stops at the first failed assertion in
each test executable.
If a module needs other source files, list them in `TEST_SUPPORT_SOURCES` in the
Makefile.
