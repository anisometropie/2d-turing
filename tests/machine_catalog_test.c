#include "machine_catalog.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

void it_should_enumerate_64_distinct_machines(void) {
  char instructions[64][7];
  MachinePreview preview;
  for (int i = 0; i < 64; i++) {
    machine_catalog_preview(1, i, &preview);
    strcpy(instructions[i], preview.instructions);
    for (int j = 0; j < i; j++)
      assert(strcmp(instructions[i], instructions[j]) != 0);
  }
  assert(strcmp(instructions[0], "0LA0LA") == 0);
  assert(strcmp(instructions[24], "1RA0LA") == 0);
  assert(strcmp(instructions[63], "1RH1RH") == 0);
}

void it_should_preview_a_machine_writing_right(void) {
  MachinePreview preview;
  machine_catalog_preview(1, 24, &preview); // 1RA0LA

  assert(preview.steps == 24);
  assert(preview.halted == false);
  assert(preview.heads[0] == 0);
  assert(preview.heads[24] == 24);
  // Column 24 is logical tape cell 0.
  assert(preview.cells[0][24] == 0);
  assert(preview.cells[1][24] == 1);
  assert(preview.cells[24][23] == 0);
  assert(preview.cells[24][24] == 1);
  assert(preview.cells[24][47] == 1);
  assert(preview.cells[24][48] == 0);
}

void it_should_preview_a_machine_writing_left(void) {
  MachinePreview preview;
  machine_catalog_preview(1, 16, &preview); // 1LA0LA

  assert(preview.steps == 24);
  assert(preview.heads[24] == -24);
  assert(preview.cells[24][0] == 0);
  assert(preview.cells[24][1] == 1);
  assert(preview.cells[24][24] == 1);
  assert(preview.cells[24][25] == 0);
}

void it_should_stop_a_preview_on_halt(void) {
  MachinePreview preview;
  machine_catalog_preview(1, 48, &preview); // 1LH0LA

  assert(preview.steps == 1);
  assert(preview.halted == true);
  assert(preview.heads[1] == -1);
  assert(preview.cells[0][24] == 0);
  assert(preview.cells[1][24] == 1);
}

void it_should_start_each_preview_with_a_fresh_tape(void) {
  MachinePreview preview;
  machine_catalog_preview(1, 24, &preview);
  machine_catalog_preview(1, 0, &preview); // 0LA0LA

  assert(preview.cells[0][24] == 0);
  assert(preview.cells[24][24] == 0);
  assert(preview.heads[24] == -24);
}

static int compare_instructions(const void *left, const void *right) {
  return strcmp(left, right);
}

void it_should_enumerate_all_two_state_machines(void) {
  assert(machine_catalog_count(1) == 64);
  assert(machine_catalog_count(2) == 20736);

  char instructions[20736][14];
  MachinePreview preview;
  for (int i = 0; i < 20736; i++) {
    machine_catalog_preview(2, i, &preview);
    strcpy(instructions[i], preview.instructions);
  }
  assert(strcmp(instructions[0], "0LA0LA_0LA0LA") == 0);
  assert(strcmp(instructions[20735], "1RH1RH_1RH1RH") == 0);

  qsort(instructions, 20736, sizeof instructions[0], compare_instructions);
  for (int i = 1; i < 20736; i++)
    assert(strcmp(instructions[i - 1], instructions[i]) != 0);
}

void it_should_preview_a_transition_through_state_b(void) {
  MachinePreview preview;
  machine_catalog_preview(2, 12216, &preview); // 1RB0LA_1LH0LA

  assert(strcmp(preview.instructions, "1RB0LA_1LH0LA") == 0);
  assert(preview.steps == 2);
  assert(preview.halted == true);
  assert(preview.heads[1] == 1);
  assert(preview.heads[2] == 0);
  assert(preview.cells[1][25] == 0);
  assert(preview.cells[2][24] == 1);
  assert(preview.cells[2][25] == 1);
}

int main(void) {
  it_should_enumerate_all_two_state_machines();
  it_should_preview_a_transition_through_state_b();
  it_should_enumerate_64_distinct_machines();
  it_should_preview_a_machine_writing_right();
  it_should_preview_a_machine_writing_left();
  it_should_stop_a_preview_on_halt();
  it_should_start_each_preview_with_a_fresh_tape();
  return 0;
}
