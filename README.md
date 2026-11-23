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
Click Restart to reset the selected machine and its history to step 0, paused.
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

## Machine galleries

Run `./2d-turing --one-state` for an 8 by 8 grid of all 64 rule tables. Each
transition can write 0 or 1, move left or right, and return to A or halt at H:
8 choices for each of the two symbols, giving 64 machines. H is not counted as
a working state. Mirror images and rules with identical behaviour are included.

Each tile shows its instructions and the first 24 steps on a zero-filled tape,
or stops earlier if it halts. Black is 1, white is 0, and red marks the head on
each row. Click a tile to open the normal viewer, paused at the last preview
step with the full preview history already shown.
Use Step or Space to explore it further; click Back to Grid or press Escape to
return. Closing the window exits the application.

Many previews are identical: on a blank tape, a one-state machine either halts
after one step or keeps moving in one direction into untouched zeros. Its rule
for reading 1 is never reached. Rows share the rule for 0; columns vary the rule
for 1.

Run `./2d-turing --two-state` for all 20,736 two-state rule tables. With working
states A and B plus halt H, each of the four rules has 12 choices, giving
12^4 machines. These include rules where B is never reached.

The two-state gallery has 324 pages of 64 previews. Use First/Prev/Next/Last,
Page Up/Page Down (or Left/Right), and Home/End to browse. Returning from a
machine keeps the current page. The 1 State and 2 States buttons switch galleries.
Each preview stops at 24 steps or halt; a 24-step preview does not establish
whether the machine eventually halts. Open a tile to run it further.

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
