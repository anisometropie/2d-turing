#include "turing.h"
#include "array.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

void it_should_initialise_turing_machine_and_store_instructions(void) {
  const TuringMachine *machine = turing_init("1RA0RA", false);
  assert(strcmp(machine->instructions, "1RA0RA") == 0);
  assert(machine->tape[0] == 0);
  assert(machine->state == 'A');
  assert(machine->head == 0);
}

typedef struct {
  const char *name;
  const char *instructions;
  bool ones;
  unsigned char expected_symbol;
  int64_t expected_head;
  char expected_state;
} StepCase;

static const StepCase cases[] = {
  // always does state A reading 0, right after initialisation
  //                   Setup                        expected
  // Name            Instructions      Ones   Symbol   Head  State
  { "writes one",    "1RA0RA",         false,   1,       1,    'A' },
  { "writes zero",   "0RA0RA",         false,   0,       1,    'A' },
  { "moves left",    "1LA0LA",         false,   1,      -1,    'A' },
  { "changes state", "1RB0RB_1RA0RA",  false,   1,       1,    'B' },
  { "start A 1",     "1RB0RB_1RA0RA",  true,    0,       1,    'B' },
  { "start A 1 b",   "1RB0LA_1RA0RA",  true,    0,       -1,   'A' },
};

void step_should_execute_one_instruction(void) {
  for (size_t i = 0; i < ARRAY_LEN(cases); i++) {
    const TuringMachine *machine = turing_init(cases[i].instructions, cases[i].ones);
    step();

    assert(machine->tape[0] == cases[i].expected_symbol);
    assert(machine->head == cases[i].expected_head);
    assert(machine->state == cases[i].expected_state);
  }
}


void more_steps(void) {
  const TuringMachine *machine = turing_init("1RB0RB_1LA0RH", false);

  step();
  assert(machine->tape[0] == 1);
  assert(machine->head == 1);
  assert(machine->state == 'B');

  step();
  assert(machine->tape[1] == 1);
  assert(machine->head == 0);
  assert(machine->state == 'A');

  step();
  assert(machine->tape[0] == 0);
  assert(machine->head == 1);
  assert(machine->state == 'B');

  step();
  assert(machine->tape[1] == 0);
  assert(machine->head == 2);
  assert(machine->state == 'H');
}

void step_should_execute_state_c_instruction(void) {
  const TuringMachine *machine = turing_init("1RB0RB_1RC0RC_0LA1RH", false);

  step();
  assert(machine->state == 'B');

  step();
  assert(machine->head == 2);
  assert(machine->state == 'C');

  step();
  assert(machine->tape[2] == 0);
  assert(machine->head == 1);
  assert(machine->state == 'A');
}

void halted_machine_should_do_nothing() {
  const TuringMachine *machine = turing_init("1RH0RB", false);

  StepResult result = step();
  assert(result == STEP_HALTED);
  assert(machine->tape[0] == 1);
  assert(machine->head == 1);
  assert(machine->state == 'H');


  result = step();
  assert(result == STEP_HALTED);
  assert(machine->tape[1] == 0);
  assert(machine->head == 1);
  assert(machine->state == 'H');
}

int main(void) {
  it_should_initialise_turing_machine_and_store_instructions();
  step_should_execute_one_instruction();
  more_steps();
  step_should_execute_state_c_instruction();
  halted_machine_should_do_nothing();
  return 0;
}
