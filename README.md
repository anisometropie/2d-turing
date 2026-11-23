# 2d-turing

A small SDL2 visualization of a Turing machine's tape.

Install SDL2 development headers and `pkg-config` first (on Debian or Ubuntu:
`sudo apt install libsdl2-dev pkg-config`). Then build with `make` and run with
`./2d-turing`. Black squares represent 1 and white squares represent 0.
The first row is the initial tape; each step adds a row underneath it. The head
on the last visible row has a red outline. Columns line up across the history.
Cells automatically shrink to fit more rows and the visited tape range. At one
pixel per cell, the view follows the newest rows; older steps remain available
by scrolling.

The machine starts paused, with no fixed step limit when running. Drag the speed
slider from 1 to 10,000 steps per second. Execution runs in batches so the UI
remains responsive at high speeds. Press Space to pause or resume. Click Step or
press Right Arrow to pause and take one step.
Scroll the mouse wheel to browse earlier rows. While browsing, the row position,
horizontal alignment, and zoom stay fixed as new steps run. Shift+wheel pans
horizontally. Home shows the earliest page; End returns to the live view and
resumes automatic fitting. All rows are retained until you close the window.
The toolbar and title show the speed, step, state, and head position. The tape
grows automatically in either direction. History stores each step's change
instead of copying the entire tape; its memory usage still grows with the number
of steps. Machines that enter H halt normally, and missing transitions stop with
a message.

To visualize another machine, pass its instructions on the command line:

```sh
./2d-turing '1RB1RH_0RC1RB_1LC1LA'
```

Press Escape or use the close button to exit. Use `make clean` to remove the executable.

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
