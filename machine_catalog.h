#ifndef MACHINE_CATALOG_H
#define MACHINE_CATALOG_H

#include <stdbool.h>

enum {
  PREVIEW_STEPS = 24,
  PREVIEW_COLUMNS = 2 * PREVIEW_STEPS + 1
};

typedef struct {
  char instructions[14];
  unsigned char cells[PREVIEW_STEPS + 1][PREVIEW_COLUMNS];
  int heads[PREVIEW_STEPS + 1];
  int steps;
  bool halted;
} MachinePreview;

// Supports one or two working states, plus halt H.
int machine_catalog_count(int states);
// Index must be less than the count. Each preview starts on a fresh zero tape.
void machine_catalog_preview(int states, int index, MachinePreview *preview);

#endif
