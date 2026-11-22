#include "turing.h"
#include "digit.h"

#include <string.h>

static int capacity = 1024;
static TuringMachine machine;

void step() {
  int offset = 0;
  if (machine.state == 'B') {
    offset += 7;
  }
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
  machine = (TuringMachine) {
    .instructions = instructions,
    .buffer = buffer,
    .tape = buffer + capacity / 2,
    .state = starting_state,
    .head = 0,
    .capacity = capacity
  };
  if (!machine.tape) {
    abort();
  }
  return &machine;
}
