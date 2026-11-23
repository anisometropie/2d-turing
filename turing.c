#include "turing.h"
#include "digit.h"

#include <string.h>

static int capacity = 1024;
static TuringMachine machine;

static void grow_tape_if_needed(void) {
  size_t origin = (size_t)(machine.tape - machine.buffer);
  if (machine.head >= -(int64_t)origin &&
      machine.head < (int64_t)(machine.capacity - origin))
    return;

  if (machine.capacity > SIZE_MAX / 2)
    abort();
  size_t new_capacity = machine.capacity * 2;
  size_t shift = machine.capacity / 2;
  unsigned char *buffer = malloc(new_capacity);
  if (buffer == NULL)
    abort();
  memset(buffer, machine.blank_symbol, new_capacity);
  memcpy(buffer + shift, machine.buffer, machine.capacity);
  free(machine.buffer);
  machine.buffer = buffer;
  machine.tape = buffer + origin + shift;
  machine.capacity = new_capacity;
}

StepResult step() {
  if (machine.state == 'H') {
    return STEP_HALTED;
  }
  int offset = (machine.state - 'A') * 7;
  if (machine.tape[machine.head] == 1) {
    offset += 3;
  }
  int write_index = offset;
  int move_index = offset + 1;
  int state_index = offset + 2;
  machine.tape[machine.head] = DIGIT_TO_INT(machine.instructions[write_index]);
  if (machine.instructions[move_index] == 'R') {
    machine.head += 1;
  } else {
    machine.head -= 1;
  }
  machine.state = machine.instructions[state_index];
  grow_tape_if_needed();
  if (machine.state == 'H') {
    return STEP_HALTED;
  }
  return STEP_RUNNING;
}

const TuringMachine *turing_init(const char *instructions, bool fill_with_ones) {
  // calloc initializes every allocated byte to 0.
  return turing_init_with_state(instructions, fill_with_ones, 'A');
}

const TuringMachine *turing_init_with_state(const char *instructions, bool fill_with_ones, char starting_state) {
  // calloc initializes every allocated byte to 0.
  unsigned char *buffer = calloc(capacity, sizeof *buffer);
  if (!buffer) {
    abort();
  }
  if (fill_with_ones) {
    memset(buffer, 1, capacity);
  }
  free(machine.buffer);
  machine = (TuringMachine) {
    .instructions = instructions,
    .buffer = buffer,
    .tape = buffer + capacity / 2,
    .state = starting_state,
    .head = 0,
    .capacity = capacity,
    .blank_symbol = fill_with_ones ? 1 : 0
  };
  if (!machine.tape) {
    abort();
  }
  return &machine;
}
