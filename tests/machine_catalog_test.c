#include "machine_catalog.h"
#include "state_name.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

static unsigned long small_count(unsigned long states) {
  mpz_t count;
  mpz_init(count);
  assert(machine_catalog_count(states, count));
  assert(mpz_fits_ulong_p(count));
  unsigned long result = mpz_get_ui(count);
  mpz_clear(count);
  return result;
}

static int small_jump(unsigned long states, unsigned long index, int symbols, int direction) {
  mpz_t source, destination;
  mpz_init_set_ui(source, index);
  mpz_init(destination);
  int result = machine_catalog_jump(states, source, symbols, direction, destination)
    ? (int)mpz_get_ui(destination) : -1;
  mpz_clears(source, destination, NULL);
  return result;
}

static void preview_at(unsigned long states, unsigned long index, MachinePreview *preview) {
  mpz_t source;
  mpz_init_set_ui(source, index);
  assert(machine_catalog_preview(states, source, preview));
  mpz_clear(source);
}

void it_should_enumerate_64_distinct_machines(void) {
  char instructions[64][7];
  MachinePreview preview = {0};
  for (int i = 0; i < 64; i++) {
    preview_at(1, i, &preview);
    strcpy(instructions[i], preview.instructions);
    for (int j = 0; j < i; j++)
      assert(strcmp(instructions[i], instructions[j]) != 0);
  }
  assert(strcmp(instructions[0], "0LA0LA") == 0);
  assert(strcmp(instructions[48], "1RA0LA") == 0);
  assert(strcmp(instructions[63], "1RH1RH") == 0);
  machine_preview_clear(&preview);
}

void it_should_preview_a_machine_writing_right(void) {
  MachinePreview preview = {0};
  preview_at(1, 48, &preview); // 1RA0LA

  assert(preview.steps == 24);
  assert(preview.halted == false);
  assert(preview.heads[0] == 0);
  assert(preview.heads[24] == 24);
  // Column 24 is logical tape cell 0.
  assert(preview.cells[0][24] == 0);
  assert(preview.cells[1][24] == 1);
  assert(preview.cells[24][23] == 0);
  assert(preview.cells[24][24] == 1);
  assert(preview.cells[24][47] == 1);
  assert(preview.cells[24][48] == 0);
  machine_preview_clear(&preview);
}

void it_should_preview_a_machine_writing_left(void) {
  MachinePreview preview = {0};
  preview_at(1, 32, &preview); // 1LA0LA

  assert(preview.steps == 24);
  assert(preview.heads[24] == -24);
  assert(preview.cells[24][0] == 0);
  assert(preview.cells[24][1] == 1);
  assert(preview.cells[24][24] == 1);
  assert(preview.cells[24][25] == 0);
  machine_preview_clear(&preview);
}

void it_should_stop_a_preview_on_halt(void) {
  MachinePreview preview = {0};
  preview_at(1, 40, &preview); // 1LH0LA

  assert(preview.steps == 1);
  assert(preview.halted == true);
  assert(preview.heads[1] == -1);
  assert(preview.cells[0][24] == 0);
  assert(preview.cells[1][24] == 1);
  machine_preview_clear(&preview);
}

void it_should_start_each_preview_with_a_fresh_tape(void) {
  MachinePreview preview = {0};
  preview_at(1, 48, &preview);
  preview_at(1, 0, &preview); // 0LA0LA

  assert(preview.cells[0][24] == 0);
  assert(preview.cells[24][24] == 0);
  assert(preview.heads[24] == -24);
  machine_preview_clear(&preview);
}

static int compare_instructions(const void *left, const void *right) {
  return strcmp(left, right);
}

void it_should_enumerate_all_two_state_machines(void) {
  assert(small_count(1) == 64);
  assert(small_count(2) == 20736);

  char instructions[20736][14];
  MachinePreview preview = {0};
  for (int i = 0; i < 20736; i++) {
    preview_at(2, i, &preview);
    strcpy(instructions[i], preview.instructions);
  }
  assert(strcmp(instructions[0], "0LA0LA_0LA0LA") == 0);
  assert(strcmp(instructions[20735], "1RH1RH_1RH1RH") == 0);

  qsort(instructions, 20736, sizeof instructions[0], compare_instructions);
  for (int i = 1; i < 20736; i++)
    assert(strcmp(instructions[i - 1], instructions[i]) != 0);
  machine_preview_clear(&preview);
}

void it_should_preview_a_transition_through_state_b(void) {
  MachinePreview preview = {0};
  preview_at(2, 17376, &preview); // 1RB0LA_1LH0LA

  assert(strcmp(preview.instructions, "1RB0LA_1LH0LA") == 0);
  assert(preview.steps == 2);
  assert(preview.halted == true);
  assert(preview.heads[1] == 1);
  assert(preview.heads[2] == 0);
  assert(preview.cells[1][25] == 0);
  assert(preview.cells[2][24] == 1);
  assert(preview.cells[2][25] == 1);
  machine_preview_clear(&preview);
}

void it_should_jump_to_the_next_leftmost_three_symbols(void) {
  int next = small_jump(2, 64, 3, 1);
  assert(next == 1728);
  MachinePreview preview = {0};
  preview_at(2, next, &preview);
  assert(strcmp(preview.instructions, "0LB0LA_0LA0LA") == 0);
  assert(small_jump(2, next, 3, 1) == 3456);
  assert(small_jump(2, next, 3, -1) == 0);
  machine_preview_clear(&preview);
}

void it_should_jump_to_the_next_leftmost_six_symbols(void) {
  int next = small_jump(2, 64, 6, 1);
  assert(next == 144);
  MachinePreview preview = {0};
  preview_at(2, next, &preview);
  assert(strcmp(preview.instructions, "0LA0LB_0LA0LA") == 0);
  // The group begins inside page 3, not at the page's first machine (128).
  assert(next / 64 == 2);
  assert(small_jump(2, next, 6, 1) == 288);
  assert(small_jump(2, 288, 6, -1) == 144);
  assert(small_jump(2, next, 6, -1) == 0);
  machine_preview_clear(&preview);
}

void it_should_stop_group_jumps_at_both_ends(void) {
  assert(small_jump(2, 0, 3, -1) == -1);
  assert(small_jump(2, 64, 6, -1) == -1);
  assert(small_jump(2, 19008, 3, 1) == -1);
  assert(small_jump(2, 20592, 6, 1) == -1);
  assert(small_jump(2, 20735, 6, 1) == -1);
}

void it_should_jump_between_groups_on_the_one_state_page(void) {
  assert(small_jump(1, 0, 3, 1) == 8);
  assert(small_jump(1, 8, 3, -1) == 0);
  assert(small_jump(1, 0, 6, 1) == 1);
  assert(small_jump(1, 1, 6, -1) == 0);
  assert(small_jump(1, 63, 6, 1) == -1);
}

void it_should_increment_the_rightmost_character_first(void) {
  const char *expected[] = {
    "0LA0LA_0LA0LA", "0LA0LA_0LA0LB", "0LA0LA_0LA0LH",
    "0LA0LA_0LA0RA", "0LA0LA_0LA0RB", "0LA0LA_0LA0RH",
    "0LA0LA_0LA1LA", "0LA0LA_0LA1LB", "0LA0LA_0LA1LH",
    "0LA0LA_0LA1RA", "0LA0LA_0LA1RB", "0LA0LA_0LA1RH"
  };
  MachinePreview preview = {0};
  for (int i = 0; i < 12; i++) {
    preview_at(2, i, &preview);
    assert(strcmp(preview.instructions, expected[i]) == 0);
  }
  preview_at(2, 12, &preview);
  assert(strcmp(preview.instructions, "0LA0LA_0LB0LA") == 0);
  preview_at(2, 144, &preview);
  assert(strcmp(preview.instructions, "0LA0LB_0LA0LA") == 0);
  preview_at(2, 1728, &preview);
  assert(strcmp(preview.instructions, "0LB0LA_0LA0LA") == 0);
  machine_preview_clear(&preview);
}

void it_should_enumerate_three_state_machine_boundaries(void) {
  assert(small_count(3) == 16777216);
  MachinePreview preview = {0};
  preview_at(3, 0, &preview);
  assert(strcmp(preview.instructions, "0LA0LA_0LA0LA_0LA0LA") == 0);
  preview_at(3, 2, &preview);
  assert(strcmp(preview.instructions, "0LA0LA_0LA0LA_0LA0LC") == 0);
  preview_at(3, 3, &preview);
  assert(strcmp(preview.instructions, "0LA0LA_0LA0LA_0LA0LH") == 0);
  preview_at(3, 16, &preview);
  assert(strcmp(preview.instructions, "0LA0LA_0LA0LA_0LB0LA") == 0);
  preview_at(3, 16777215, &preview);
  assert(strcmp(preview.instructions, "1RH1RH_1RH1RH_1RH1RH") == 0);
  machine_preview_clear(&preview);
}

void it_should_preview_a_three_state_machine_through_state_c(void) {
  MachinePreview preview = {0};
  preview_at(3, 13689008, &preview); // 1RB0LA_1RC0LA_1LH0LA

  assert(strcmp(preview.instructions, "1RB0LA_1RC0LA_1LH0LA") == 0);
  assert(preview.steps == 3);
  assert(preview.halted == true);
  assert(preview.heads[1] == 1);
  assert(preview.heads[2] == 2);
  assert(preview.heads[3] == 1);
  assert(preview.cells[2][26] == 0);
  assert(preview.cells[3][24] == 1);
  assert(preview.cells[3][25] == 1);
  assert(preview.cells[3][26] == 1);
  machine_preview_clear(&preview);
}

void it_should_jump_between_three_state_prefix_groups(void) {
  assert(small_jump(3, 0, 3, 1) == 1048576);
  assert(small_jump(3, 1048576, 3, -1) == 0);
  assert(small_jump(3, 0, 6, 1) == 65536);
  assert(small_jump(3, 65536, 6, -1) == 0);
  assert(small_jump(3, 0, 3, -1) == -1);
  assert(small_jump(3, 15728640, 3, 1) == -1);
  assert(small_jump(3, 16711680, 6, 1) == -1);

  MachinePreview preview = {0};
  preview_at(3, 1048576, &preview);
  assert(strcmp(preview.instructions, "0LB0LA_0LA0LA_0LA0LA") == 0);
  preview_at(3, 65536, &preview);
  assert(strcmp(preview.instructions, "0LA0LB_0LA0LA_0LA0LA") == 0);
  machine_preview_clear(&preview);
}

void it_should_count_and_navigate_beyond_64_bit_indexes(void) {
  mpz_t count, index, previous;
  mpz_inits(count, index, previous, NULL);
  assert(machine_catalog_count(8, count));
  char *decimal = mpz_get_str(NULL, 10, count);
  assert(strcmp(decimal, "7958661109946400884391936") == 0);
  free(decimal);

  mpz_sub_ui(index, count, 1);
  MachinePreview preview = {0};
  assert(machine_catalog_preview(8, index, &preview));
  assert(strcmp(preview.instructions,
    "1RH1RH_1RH1RH_1RH1RH_1RH1RH_1RH1RH_1RH1RH_1RH1RH_1RH1RH") == 0);
  assert(preview.halted == true);
  assert(machine_catalog_jump(8, index, 6, -1, previous));
  assert(machine_catalog_jump(8, previous, 6, 1, index));
  assert(!machine_catalog_jump(8, index, 6, 1, previous));
  machine_preview_clear(&preview);
  mpz_clears(count, index, previous, NULL);
}

void it_should_generate_i_instead_of_using_h_as_a_working_state(void) {
  mpz_t index;
  mpz_init(index);
  mpz_ui_pow_ui(index, 36, 15);
  mpz_mul_ui(index, index, 7);
  MachinePreview preview = {0};
  assert(machine_catalog_preview(8, index, &preview));
  assert(strncmp(preview.instructions, "0LI0LA_", 7) == 0);
  assert(preview.halted == false);
  assert(preview.steps == 24);
  machine_preview_clear(&preview);
  mpz_clear(index);
}

void it_should_generate_and_run_state_aa(void) {
  mpz_t index;
  mpz_init(index);
  mpz_ui_pow_ui(index, 108, 51);
  mpz_mul_ui(index, index, 106); // A0: 1RAA
  mpz_add_ui(index, index, 107 * 108); // AA0: 1RH
  MachinePreview preview = {0};
  assert(machine_catalog_preview(26, index, &preview));
  assert(strncmp(preview.instructions, "1RAA0LA_", 8) == 0);
  assert(strcmp(strrchr(preview.instructions, '_'), "_1RH0LA") == 0);
  assert(preview.steps == 2);
  assert(preview.halted == true);
  assert(preview.heads[2] == 2);
  assert(preview.cells[2][24] == 1);
  assert(preview.cells[2][25] == 1);
  machine_preview_clear(&preview);
  mpz_clear(index);
}

void it_should_generate_only_requested_thousand_state_previews(void) {
  mpz_t count, index;
  mpz_inits(count, index, NULL);
  assert(machine_catalog_count(1000, count));
  char *decimal = mpz_get_str(NULL, 10, count);
  assert(strlen(decimal) == 7205);
  free(decimal);
  MachinePreview preview = {0};
  assert(machine_catalog_preview(1000, index, &preview));
  assert(strlen(preview.instructions) == 6999);
  assert(preview.steps == 24);
  assert(preview.halted == false);
  mpz_sub_ui(index, count, 1);
  assert(machine_catalog_preview(1000, index, &preview));
  assert(strlen(preview.instructions) == 6999);
  assert(preview.steps == 1);
  assert(preview.halted == true);
  machine_preview_clear(&preview);
  mpz_clears(count, index, NULL);
}

void it_should_reject_invalid_catalog_requests(void) {
  mpz_t count, index;
  mpz_inits(count, index, NULL);
  MachinePreview preview = {0};
  assert(!machine_catalog_count(0, count));
  assert(!machine_catalog_preview(0, index, &preview));
  assert(machine_catalog_count(4, count));
  assert(!machine_catalog_preview(4, count, &preview));
  mpz_set_si(index, -1);
  assert(!machine_catalog_preview(4, index, &preview));
  assert(preview.instructions == NULL);
  mpz_clears(count, index, NULL);
}

void it_should_parse_positive_state_counts(void) {
  unsigned long states = 0;
  assert(machine_catalog_parse_states("26", &states));
  assert(states == 26);
  assert(machine_catalog_parse_states("1000", &states));
  assert(states == 1000);
  assert(!machine_catalog_parse_states("0", &states));
  assert(!machine_catalog_parse_states("-2", &states));
  assert(!machine_catalog_parse_states("", &states));
  assert(!machine_catalog_parse_states("2x", &states));
  assert(!machine_catalog_parse_states("999999999999999999999999999", &states));
}

int main(void) {
  it_should_parse_positive_state_counts();
  it_should_count_and_navigate_beyond_64_bit_indexes();
  it_should_generate_i_instead_of_using_h_as_a_working_state();
  it_should_generate_and_run_state_aa();
  it_should_generate_only_requested_thousand_state_previews();
  it_should_reject_invalid_catalog_requests();
  it_should_enumerate_three_state_machine_boundaries();
  it_should_preview_a_three_state_machine_through_state_c();
  it_should_jump_between_three_state_prefix_groups();
  it_should_increment_the_rightmost_character_first();
  it_should_jump_to_the_next_leftmost_three_symbols();
  it_should_jump_to_the_next_leftmost_six_symbols();
  it_should_stop_group_jumps_at_both_ends();
  it_should_jump_between_groups_on_the_one_state_page();
  it_should_enumerate_all_two_state_machines();
  it_should_preview_a_transition_through_state_b();
  it_should_enumerate_64_distinct_machines();
  it_should_preview_a_machine_writing_right();
  it_should_preview_a_machine_writing_left();
  it_should_stop_a_preview_on_halt();
  it_should_start_each_preview_with_a_fresh_tape();
  return 0;
}
