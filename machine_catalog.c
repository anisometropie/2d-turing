#include "machine_catalog.h"
#include "turing.h"

#include <limits.h>
#include <errno.h>
#include <string.h>

bool machine_catalog_parse_states(const char *text, unsigned long *states) {
  if (*text == '\0')
    return false;
  for (const char *cursor = text; *cursor; cursor++) {
    if (*cursor < '0' || *cursor > '9')
      return false;
  }
  errno = 0;
  unsigned long value = strtoul(text, NULL, 10);
  if (errno == ERANGE || value == 0 || value > ULONG_MAX / 4 - 1 || value > SIZE_MAX - 'A' - 1)
    return false;
  *states = value;
  return true;
}

bool machine_catalog_count(unsigned long states, mpz_t count) {
  if (states == 0 || states > ULONG_MAX / 4 - 1 || states > SIZE_MAX - 'A' - 1)
    return false;
  mpz_ui_pow_ui(count, 4 * (states + 1), states * 2);
  return true;
}

bool machine_catalog_jump(unsigned long states, const mpz_t index,
                           int prefix_symbols, int direction, mpz_t destination) {
  if ((prefix_symbols != 3 && prefix_symbols != 6) || (direction != -1 && direction != 1))
    return false;
  mpz_t count, group_size, target;
  mpz_inits(count, group_size, target, NULL);
  bool valid = machine_catalog_count(states, count) &&
    mpz_sgn(index) >= 0 && mpz_cmp(index, count) < 0;
  if (valid) {
    mpz_set(group_size, count);
    for (int rule = 0; rule < prefix_symbols / 3; rule++)
      mpz_divexact_ui(group_size, group_size, 4 * (states + 1));
    mpz_fdiv_q(target, index, group_size);
    if (direction > 0)
      mpz_add_ui(target, target, 1);
    else
      mpz_sub_ui(target, target, 1);
    mpz_mul(target, target, group_size);
    valid = mpz_sgn(target) >= 0 && mpz_cmp(target, count) < 0;
    if (valid)
      mpz_set(destination, target);
  }
  mpz_clears(count, group_size, target, NULL);
  return valid;
}

void machine_preview_clear(MachinePreview *preview) {
  free(preview->instructions);
  *preview = (MachinePreview){0};
}

bool machine_catalog_preview(unsigned long states, const mpz_t index, MachinePreview *preview) {
  mpz_t count, remaining;
  mpz_inits(count, remaining, NULL);
  bool valid = machine_catalog_count(states, count) &&
    mpz_sgn(index) >= 0 && mpz_cmp(index, count) < 0;
  if (!valid) {
    mpz_clears(count, remaining, NULL);
    return false;
  }

  char name[STATE_NAME_CAPACITY];
  state_format(state_from_index(states - 1), name);
  size_t block_capacity = 5 + 2 * strlen(name);
  if (states > SIZE_MAX / block_capacity || states > SIZE_MAX / (2 * sizeof(unsigned long))) {
    mpz_clears(count, remaining, NULL);
    return false;
  }
  char *instructions = malloc((size_t)states * block_capacity);
  unsigned long *rules = calloc((size_t)states * 2, sizeof *rules);
  if (instructions == NULL || rules == NULL) {
    free(instructions);
    free(rules);
    mpz_clears(count, remaining, NULL);
    return false;
  }
  unsigned long choices = 4 * (states + 1);
  mpz_set(remaining, index);
  for (size_t rule = (size_t)states * 2; rule > 0 && mpz_sgn(remaining) != 0;)
    rules[--rule] = mpz_tdiv_q_ui(remaining, remaining, choices);
  mpz_clears(count, remaining, NULL);

  char *cursor = instructions;
  for (size_t rule = 0; rule < (size_t)states * 2; rule++) {
    if (rule > 0 && rule % 2 == 0)
      *cursor++ = '_';
    unsigned long choice = rules[rule];
    unsigned long target = choice % (states + 1);
    *cursor++ = (char)('0' + choice / (2 * (states + 1)));
    *cursor++ = (choice / (states + 1)) % 2 ? 'R' : 'L';
    state_format(target == states ? 'H' : state_from_index(target), name);
    size_t length = strlen(name);
    memcpy(cursor, name, length);
    cursor += length;
  }
  *cursor = '\0';
  free(rules);

  machine_preview_clear(preview);
  preview->instructions = instructions;
  const TuringMachine *machine = turing_init(instructions, false);
  for (int row = 0; row <= PREVIEW_STEPS; row++) {
    preview->heads[row] = (int)machine->head;
    memcpy(preview->cells[row], machine->tape - PREVIEW_STEPS, PREVIEW_COLUMNS);
    preview->steps = row;
    preview->halted = machine->state == 'H';
    if (preview->halted || row == PREVIEW_STEPS)
      break;
    step();
  }
  return true;
}
