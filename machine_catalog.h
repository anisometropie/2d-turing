#ifndef MACHINE_CATALOG_H
#define MACHINE_CATALOG_H

#include <stdbool.h>
#include <gmp.h>

enum {
  PREVIEW_STEPS = 24,
  PREVIEW_COLUMNS = 2 * PREVIEW_STEPS + 1
};

typedef struct {
  char *instructions;
  unsigned char cells[PREVIEW_STEPS + 1][PREVIEW_COLUMNS];
  int heads[PREVIEW_STEPS + 1];
  int steps;
  bool halted;
} MachinePreview;

// GMP output values must be initialized by the caller.
bool machine_catalog_parse_states(const char *text, unsigned long *states);
bool machine_catalog_count(unsigned long states, mpz_t count);
// Prefix sizes 3 and 6 mean the first one or two complete transition rules.
// Return false when there is no next/previous group.
bool machine_catalog_jump(unsigned long states, const mpz_t index,
                           int prefix_symbols, int direction, mpz_t destination);
// Initialize previews with {0}. Reusing a preview replaces its owned instructions.
bool machine_catalog_preview(unsigned long states, const mpz_t index, MachinePreview *preview);
void machine_preview_clear(MachinePreview *preview);

#endif
