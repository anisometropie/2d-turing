#include "turing.h"
#include "digit.h"

#include <string.h>

static int capacity = 1024;
static TuringMachine machine;

typedef struct {
  unsigned char symbol;
  int direction;
  TuringState next_state;
  bool valid;
} Transition;

static Transition *transitions;
static size_t state_count;

static Transition *parse_instructions(const char *instructions, size_t count) {
  Transition *parsed = calloc(count * 2, sizeof *parsed);
  if (parsed == NULL)
    abort();
  const char *cursor = instructions;
  for (size_t state = 0; state < count; state++) {
    bool valid = true;
    for (int symbol = 0; symbol < 2; symbol++) {
      if ((*cursor != '0' && *cursor != '1') ||
          (cursor[1] != 'L' && cursor[1] != 'R')) {
        valid = false;
        break;
      }
      const char *name = cursor + 2;
      const char *end = name;
      while (*end >= 'A' && *end <= 'Z')
        end++;
      TuringState next;
      if (!state_parse(name, (size_t)(end - name), &next)) {
        valid = false;
        break;
      }
      parsed[state * 2 + symbol] = (Transition){
        .symbol = (unsigned char)DIGIT_TO_INT(cursor[0]),
        .direction = cursor[1] == 'R' ? 1 : -1,
        .next_state = next,
        .valid = true
      };
      cursor = end;
    }
    if (*cursor != '_' && *cursor != '\0')
      valid = false;
    if (!valid) {
      parsed[state * 2].valid = false;
      parsed[state * 2 + 1].valid = false;
    }
    while (*cursor && *cursor != '_')
      cursor++;
    if (*cursor == '_')
      cursor++;
  }
  return parsed;
}

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

bool turing_can_step(void) {
  if (machine.state == 'H' || machine.state < 'A')
    return false;
  size_t state = state_index(machine.state);
  unsigned char symbol = machine.tape[machine.head];
  return state < state_count && symbol <= 1 && transitions[state * 2 + symbol].valid;
}

StepResult step(void) {
  if (machine.state == 'H')
    return STEP_HALTED;
  if (!turing_can_step())
    return STEP_MISSING_TRANSITION;

  size_t state = state_index(machine.state);
  unsigned char symbol = machine.tape[machine.head];
  Transition instruction = transitions[state * 2 + symbol];
  machine.tape[machine.head] = instruction.symbol;
  machine.head += instruction.direction;
  machine.state = instruction.next_state;
  grow_tape_if_needed();
  return machine.state == 'H' ? STEP_HALTED : STEP_RUNNING;
}

const TuringMachine *turing_init(const char *instructions, bool fill_with_ones) {
  // calloc initializes every allocated byte to 0.
  return turing_init_with_state(instructions, fill_with_ones, 'A');
}

const TuringMachine *turing_init_with_state(const char *instructions, bool fill_with_ones, TuringState starting_state) {
  // calloc initializes every allocated byte to 0.
  unsigned char *buffer = calloc(capacity, sizeof *buffer);
  if (!buffer) {
    abort();
  }
  if (fill_with_ones) {
    memset(buffer, 1, capacity);
  }
  size_t count = 1;
  for (const char *cursor = instructions; *cursor; cursor++) {
    if (*cursor == '_')
      count++;
  }
  Transition *parsed = parse_instructions(instructions, count);
  free(transitions);
  transitions = parsed;
  state_count = count;
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
