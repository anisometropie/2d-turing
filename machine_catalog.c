#include "machine_catalog.h"
#include "turing.h"

#include <string.h>

int machine_catalog_count(int states) {
  return states == 1 ? 64 : 20736;
}

int machine_catalog_jump(int states, int index, int prefix_symbols, int direction) {
  int count = machine_catalog_count(states);
  int group_size = count;
  for (int rule = 0; rule < prefix_symbols / 3; rule++)
    group_size /= 4 * (states + 1);
  int next = (index / group_size + direction) * group_size;
  return next >= 0 && next < count ? next : -1;
}

void machine_catalog_preview(int states, int index, MachinePreview *preview) {
  *preview = (MachinePreview){0};
  int choices = 4 * (states + 1);
  for (int rule = states * 2 - 1; rule >= 0; rule--) {
    int choice = index % choices;
    index /= choices;
    int offset = (rule / 2) * 7 + (rule % 2) * 3;
    int target = choice % (states + 1);
    preview->instructions[offset] = '0' + choice / (2 * (states + 1));
    preview->instructions[offset + 1] = (choice / (states + 1)) % 2 ? 'R' : 'L';
    preview->instructions[offset + 2] = target == states ? 'H' : 'A' + target;
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
