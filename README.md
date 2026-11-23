# 2d-turing

A small SDL2 visualization of a Turing machine's tape.

Install SDL2 and GMP development headers and `pkg-config` first (on Debian or Ubuntu:
`sudo apt install libsdl2-dev libgmp-dev pkg-config`). Then build with `make` and run with
`./2d-turing`. Black squares represent 1 and white squares represent 0.
The first row is the initial tape; each step adds a row underneath it. The head
on the last visible row has a red outline. Columns line up across the history.
Columns and cell size stay fixed between steps. When the history approaches a
side or the bottom edge, cells shrink in larger increments to keep the whole
history visible and leave room for more steps. At one pixel per cell, the view
follows the newest rows and pans only when the head nears a side; older steps
remain available by scrolling.

The machine starts paused, with no fixed step limit when running. Drag the speed
slider from 1 to 10,000 steps per second. Execution runs in batches so the UI
remains responsive at high speeds. Click Play/Pause or press Space to pause or resume. Click Step or
press Right Arrow to pause and take one step.
Click Restart to reset the selected machine and its history to step 0, paused.
Check Head Trail to show a red head outline on every history row. Uncheck it to
show only the head on the last visible row.
Scroll the mouse wheel to browse earlier rows. While browsing, the row position,
horizontal alignment, and zoom stay fixed as new steps run. Shift+wheel pans
horizontally. Home shows the earliest page; End returns to the live view and
resumes automatic fitting. All rows are retained until you close the window.
Ctrl+mouse wheel zooms in or out around the pointer over the tape. Manual zoom
stays fixed while stepping or running; press End to restore automatic fitting.
The toolbar and title show the speed, step, state, and head position. The tape
grows automatically in either direction. History stores each step's change
instead of copying the entire tape; its memory usage still grows with the number
of steps. Machines that enter H halt normally, and missing transitions stop with
a message.

To visualize another machine, pass its instructions on the command line:

```sh
./2d-turing '1RB1RH_0RC1RB_1LC1LA'
```

The bbchallenge `---` notation is also accepted: encountering it halts without
writing or moving the head. For example:

```sh
./2d-turing '1RB1RA_0LC1LE_1LD1LC_1LA0LB_1LF1RE_---0RA'
```

Press Escape or use the close button to exit. Use `make clean` to remove the executable.

## Machine galleries

Run `./2d-turing --states 1` for an 8 by 8 grid of all 64 rule tables. Each
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

Run `./2d-turing --states 2` for all 20,736 two-state rule tables. With working
states A and B plus halt H, each of the four rules has 12 choices, giving
12^4 machines. These include rules where B is never reached.

The two-state gallery has 324 pages of 64 previews. Use First/Prev/Next/Last,
Page Up/Page Down (or Left/Right), and Home/End to browse. Returning from a
machine keeps the current page. Use the −/+ buttons beside States to change the
state count, or click the number, type a new count, and press Enter. Escape
cancels editing.

Run `./2d-turing --states 3` for the three-state catalog: A, B, C, and halt H
give 16 choices per rule and 16^6 = 16,777,216 rule tables across 262,144 pages.
Only the current page's 64 previews are generated. The same page and prefix
arrows work here.

For any positive number of working states, use `./2d-turing --states N`, for example:

```sh
./2d-turing --states 26
./2d-turing --states 1000
```

State names run A through G, I through Z, then AA, AB, and so on. Only the exact
name H means halt; AH and HA are working states. Instruction blocks follow that
order. For example, `1RAA` writes 1, moves right, and enters AA. Existing A/B/C
machine strings work unchanged.

An N-state catalog has `[4 × (N + 1)]^(2N)` rule tables. Catalog indexes and
navigation use arbitrary precision integers, so large catalogs do not overflow.
Only the current page's 64 machines are generated and simulated. Very large
counts still require time and memory proportional to the requested state count.
Long numbers use scientific notation on screen, and long instruction strings
are truncated to fit their tiles. Neither changes the actual machine or index.

The compact previous/next groups are `<<< << <` and `> >> >>>`. Single arrows
move one page, double arrows jump by the first two transition rules (6 symbols),
and triple arrows jump by the first transition rule (3 symbols). A multi-letter
state name such as AA counts as one state symbol. Jumps go to the first machine in the next or
previous group sharing that prefix. The destination page opens
and that machine gets a blue outline. Further jumps use the highlighted machine,
including when a group begins partway through a page. Clicking a tile also selects
it. Group jumps work within the single page of one-state machines too.

Enumeration works like a counter: the rightmost character changes fastest.
Within each rule, the next state changes first (A, B, H for two-state machines),
then direction (L, R), then the written symbol (0, 1). After a rule rolls over,
the rule to its left advances. Underscores remain fixed separators.

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
