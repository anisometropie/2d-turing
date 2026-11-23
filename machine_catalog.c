#include "machine_catalog.h"
#include "turing.h"

#include <string.h>

int machine_catalog_count(int states) {
  return states == 1 ? 64 : 20736;
}

void machine_catalog_preview(int states, int index, MachinePreview *preview) {
  *preview = (MachinePreview){0};
  int choices = 4 * (states + 1);
  for (int rule = states * 2 - 1; rule >= 0; rule--) {
    int choice = index % choices;
    index /= choices;
    int offset = (rule / 2) * 7 + (rule % 2) * 3;
    preview->instructions[offset] = '0' + (choice % 4) / 2;
    preview->instructions[offset + 1] = choice % 2 ? 'R' : 'L';
    preview->instructions[offset + 2] = choice / 4 == states ? 'H' : 'A' + choice / 4;
  }
  if (states == 2)
    preview->instructions[6] = '_';

  const TuringMachine *machine = turing_init(preview->instructions, false);
  for (int row = 0; row <= PREVIEW_STEPS; row++) {
    preview->heads[row] = (int)machine->head;
    memcpy(preview->cells[row], machine->tape - PREVIEW_STEPS, PREVIEW_COLUMNS);
    preview->steps = row;
    preview->halted = machine->state == 'H';
    if (preview->halted || row == PREVIEW_STEPS)
      break;
    step();
  }
}
