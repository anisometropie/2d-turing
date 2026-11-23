#include "state_name.h"

#include <assert.h>
#include <string.h>

void it_should_reserve_h_for_halt(void) {
  assert(state_from_index(0) == 'A');
  assert(state_from_index(6) == 'G');
  assert(state_from_index(7) == 'I');
  assert(state_index('I') == 7);
  TuringState state;
  assert(state_parse("H", 1, &state));
  assert(state == 'H');
}

void it_should_continue_with_multi_letter_names(void) {
  char name[STATE_NAME_CAPACITY];
  state_format(state_from_index(24), name);
  assert(strcmp(name, "Z") == 0);
  state_format(state_from_index(25), name);
  assert(strcmp(name, "AA") == 0);
  state_format(state_from_index(26), name);
  assert(strcmp(name, "AB") == 0);
  state_format(state_from_index(32), name);
  assert(strcmp(name, "AH") == 0);
  TuringState state;
  assert(state_parse("AA", 2, &state));
  assert(state_index(state) == 25);
  assert(state_parse("AH", 2, &state));
  assert(state != 'H');
}

void it_should_round_trip_large_state_names(void) {
  char name[STATE_NAME_CAPACITY];
  state_format(state_from_index(999), name);
  assert(strcmp(name, "ALM") == 0);
  TuringState state;
  assert(state_parse(name, strlen(name), &state));
  assert(state_index(state) == 999);
  assert(!state_parse("", 0, &state));
  assert(!state_parse("A1", 2, &state));
  assert(!state_parse("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ", 32, &state));
}

int main(void) {
  it_should_reserve_h_for_halt();
  it_should_continue_with_multi_letter_names();
  it_should_round_trip_large_state_names();
  return 0;
}
