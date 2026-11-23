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
  TuringState expected_state;
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

void step_should_grow_tape_at_right_boundary(void) {
  const TuringMachine *machine = turing_init("1RA1RA", false);
  size_t initial_capacity = machine->capacity;

  // The initial tape runs from -512 to 511.
  for (int i = 0; i < 511; i++) {
    step();
  }

  assert(machine->head == 511);
  assert(machine->capacity == initial_capacity);

  step();

  assert(machine->head == 512);
  assert(machine->capacity == 2048);
  assert(machine->tape[0] == 1);
  assert(machine->tape[511] == 1);
  assert(machine->tape[512] == 0);
}

void step_should_execute_a_multi_letter_state(void) {
  char instructions[256] = "1RAA0LA";
  for (int state = 1; state < 25; state++)
    strcat(instructions, "_0LA0LA");
  strcat(instructions, "_1RH0LA");
  const TuringMachine *machine = turing_init(instructions, false);

  step();
  assert(machine->state == state_from_index(25));
  assert(machine->head == 1);
  StepResult result = step();
  assert(result == STEP_HALTED);
  assert(machine->state == 'H');
  assert(machine->head == 2);
  assert(machine->tape[1] == 1);
}

void step_should_use_i_as_the_eighth_working_state(void) {
  const TuringMachine *machine = turing_init(
    "1RI0LA_0LA0LA_0LA0LA_0LA0LA_0LA0LA_0LA0LA_0LA0LA_1RH0LA", false);
  step();
  assert(machine->state == 'I');
  StepResult result = step();
  assert(result == STEP_HALTED);
  assert(machine->head == 2);
}

void step_should_report_an_undefined_multi_letter_state(void) {
  const TuringMachine *machine = turing_init("1RAA0LA", false);
  assert(step() == STEP_RUNNING);
  assert(machine->state == state_from_index(25));
  assert(step() == STEP_MISSING_TRANSITION);
  assert(machine->head == 1);
  assert(machine->tape[1] == 0);
}

void step_should_halt_on_an_undefined_bbchallenge_transition(void) {
  const char *instructions = "1RB1RA_0LC1LE_1LD1LC_1LA0LB_1LF1RE_---0RA";
  const TuringMachine *machine = turing_init_with_state(instructions, false, 'F');

  assert(turing_can_step());
  assert(step() == STEP_HALTED);
  assert(machine->state == 'H');
  assert(machine->head == 0);
  assert(machine->tape[0] == 0);
  assert(step() == STEP_HALTED);
  assert(machine->head == 0);
}

void step_should_keep_the_rule_beside_an_undefined_transition(void) {
  const char *instructions = "1RB1RA_0LC1LE_1LD1LC_1LA0LB_1LF1RE_---0RA";
  const TuringMachine *machine = turing_init_with_state(instructions, true, 'F');

  assert(step() == STEP_RUNNING);
  assert(machine->state == 'A');
  assert(machine->head == 1);
  assert(machine->tape[0] == 0);
}

void step_should_accept_undefined_rules_in_either_slot(void) {
  const TuringMachine *machine = turing_init("1RA---", true);
  assert(step() == STEP_HALTED);
  assert(machine->head == 0);
  assert(machine->tape[0] == 1);

  machine = turing_init("------", false);
  assert(step() == STEP_HALTED);
  assert(machine->head == 0);
  assert(machine->tape[0] == 0);
}

int main(void) {
  step_should_halt_on_an_undefined_bbchallenge_transition();
  step_should_keep_the_rule_beside_an_undefined_transition();
  step_should_accept_undefined_rules_in_either_slot();
  step_should_report_an_undefined_multi_letter_state();
  step_should_execute_a_multi_letter_state();
  step_should_use_i_as_the_eighth_working_state();
  it_should_initialise_turing_machine_and_store_instructions();
  step_should_execute_one_instruction();
  more_steps();
  step_should_execute_state_c_instruction();
  halted_machine_should_do_nothing();
  step_should_grow_tape_at_right_boundary();
  return 0;
}
