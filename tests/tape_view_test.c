#include "tape_view.h"

#include <assert.h>
#include <string.h>

static SDL_Renderer *renderer;

static void close_after_first_frame(void) {
  SDL_Event event = {.type = SDL_QUIT};
  SDL_PushEvent(&event);
}

void it_should_open_with_the_preview_steps_already_done(void) {
  const TuringMachine *machine = turing_init("1RA0LA", false);
  close_after_first_frame();
  TapeViewResult result = tape_view_run(renderer, machine, true, 24);

  assert(result == TAPE_VIEW_EXIT);
  assert(machine->head == 24);
  assert(machine->tape[0] == 1);
  assert(machine->tape[23] == 1);
  assert(machine->tape[24] == 0);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "rows 0-24") != NULL);
  assert(strstr(title, "step 24/24") != NULL);
  assert(strstr(title, "PAUSED") != NULL);
}

void it_should_continue_after_the_preview(void) {
  const TuringMachine *machine = turing_init("1RA0LA", false);
  SDL_Event event = {.type = SDL_KEYDOWN};
  event.key.keysym.sym = SDLK_RIGHT;
  SDL_PushEvent(&event);
  close_after_first_frame();
  tape_view_run(renderer, machine, true, 24);

  assert(machine->head == 25);
  assert(machine->tape[24] == 1);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "step 25/25") != NULL);
}

void it_should_keep_a_halted_preview_halted(void) {
  const TuringMachine *machine = turing_init("1RB0LA_1LH0LA", false);
  close_after_first_frame();
  tape_view_run(renderer, machine, true, 2);

  assert(machine->state == 'H');
  assert(machine->head == 0);
  assert(machine->tape[0] == 1);
  assert(machine->tape[1] == 1);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "rows 0-2") != NULL);
  assert(strstr(title, "step 2/2") != NULL);
  assert(strstr(title, "HALTED") != NULL);
}

void it_should_still_start_standalone_machines_at_zero(void) {
  const TuringMachine *machine = turing_init("1RA0LA", false);
  close_after_first_frame();
  tape_view_run(renderer, machine, false, 0);

  assert(machine->head == 0);
  assert(machine->tape[0] == 0);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "step 0/0") != NULL);
  assert(strstr(title, "PAUSED") != NULL);
}

static void click_restart(void) {
  SDL_Event event = {.type = SDL_MOUSEBUTTONDOWN};
  event.button.button = SDL_BUTTON_LEFT;
  event.button.x = 780;
  event.button.y = 18;
  SDL_PushEvent(&event);
}

void it_should_restart_the_selected_machine_and_clear_history(void) {
  const TuringMachine *machine = turing_init("1RA0LA", false);
  SDL_Event event = {.type = SDL_KEYDOWN};
  event.key.keysym.sym = SDLK_SPACE;
  SDL_PushEvent(&event);
  click_restart();
  close_after_first_frame();
  tape_view_run(renderer, machine, true, 24);

  assert(strcmp(machine->instructions, "1RA0LA") == 0);
  assert(machine->head == 0);
  assert(machine->state == 'A');
  assert(machine->tape[0] == 0);
  assert(machine->tape[23] == 0);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "rows 0-0") != NULL);
  assert(strstr(title, "step 0/0") != NULL);
  assert(strstr(title, "PAUSED") != NULL);
  assert(strstr(title, "LIVE") != NULL);
}

void it_should_allow_stepping_after_restarting_a_halted_machine(void) {
  const TuringMachine *machine = turing_init("1RB0LA_1LH0LA", false);
  click_restart();
  SDL_Event event = {.type = SDL_KEYDOWN};
  event.key.keysym.sym = SDLK_RIGHT;
  SDL_PushEvent(&event);
  close_after_first_frame();
  tape_view_run(renderer, machine, true, 2);

  assert(machine->state == 'B');
  assert(machine->head == 1);
  assert(machine->tape[0] == 1);
  assert(machine->tape[1] == 0);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "step 1/1") != NULL);
  assert(strstr(title, "PAUSED") != NULL);
}

static void click_head_trail(void) {
  SDL_Event event = {.type = SDL_MOUSEBUTTONDOWN};
  event.button.button = SDL_BUTTON_LEFT;
  event.button.x = 270;
  event.button.y = 58;
  SDL_PushEvent(&event);
}

static Uint32 pixel_at(int x, int y) {
  SDL_Rect area = {x, y, 1, 1};
  Uint32 pixel;
  int result = SDL_RenderReadPixels(renderer, &area, SDL_PIXELFORMAT_ARGB8888,
                                    &pixel, sizeof pixel);
  assert(result == 0);
  return pixel;
}

void it_should_show_the_head_on_each_row_when_checked(void) {
  const TuringMachine *machine = turing_init("1RA0LA", false);
  click_head_trail();
  close_after_first_frame();
  tape_view_run(renderer, machine, true, 2);

  // At 1024x768, these are the head borders for steps 0, 1, and 2.
  assert(pixel_at(492, 84) == 0xffdc4123);
  assert(pixel_at(512, 104) == 0xffdc4123);
  assert(pixel_at(532, 124) == 0xffdc4123);
  assert(machine->head == 2);
  assert(machine->tape[0] == 1);
  assert(machine->tape[1] == 1);
}

void it_should_hide_past_heads_when_unchecked(void) {
  const TuringMachine *machine = turing_init("1RA0LA", false);
  click_head_trail();
  click_head_trail();
  close_after_first_frame();
  tape_view_run(renderer, machine, true, 2);

  assert(pixel_at(492, 84) == 0xffffffff);
  assert(pixel_at(512, 104) == 0xffffffff);
  assert(pixel_at(532, 124) == 0xffdc4123);
}

static void click_play_pause(void) {
  SDL_Event event = {.type = SDL_MOUSEBUTTONDOWN};
  event.button.button = SDL_BUTTON_LEFT;
  event.button.x = 880;
  event.button.y = 18;
  SDL_PushEvent(&event);
}

void it_should_toggle_play_and_pause(void) {
  const TuringMachine *machine = turing_init("1RA0LA", false);
  click_play_pause();
  close_after_first_frame();
  tape_view_run(renderer, machine, true, 24);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "RUNNING") != NULL);

  machine = turing_init("1RA0LA", false);
  click_play_pause();
  click_play_pause();
  close_after_first_frame();
  tape_view_run(renderer, machine, true, 24);
  title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "PAUSED") != NULL);
}

static void queue_steps(int count) {
  SDL_Event event = {.type = SDL_KEYDOWN};
  event.key.keysym.sym = SDLK_RIGHT;
  for (int i = 0; i < count; i++)
    SDL_PushEvent(&event);
}

void it_should_keep_earlier_heads_in_place_while_stepping(void) {
  const TuringMachine *machine = turing_init("1RA0LA", false);
  click_head_trail();
  queue_steps(2);
  close_after_first_frame();
  tape_view_run(renderer, machine, false, 0);

  // Cell 0 starts at x=512 and must stay there when steps 1 and 2 are added.
  assert(pixel_at(512, 84) == 0xffdc4123);
  assert(pixel_at(532, 104) == 0xffdc4123);
  assert(pixel_at(552, 124) == 0xffdc4123);
}

void it_should_wait_until_near_the_right_edge_before_fitting(void) {
  const TuringMachine *machine = turing_init("1RA1RA", false);
  queue_steps(18);
  close_after_first_frame();
  tape_view_run(renderer, machine, false, 0);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "cell 20 | left -25") != NULL);

  machine = turing_init("1RA1RA", false);
  queue_steps(20);
  close_after_first_frame();
  tape_view_run(renderer, machine, false, 0);
  title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "rows 0-20") != NULL);
  assert(strstr(title, "cell 15 | left -23") != NULL);
}

void it_should_fit_when_near_the_left_edge(void) {
  const TuringMachine *machine = turing_init("1LA1LA", false);
  queue_steps(21);
  close_after_first_frame();
  tape_view_run(renderer, machine, false, 0);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "rows 0-21") != NULL);
  assert(strstr(title, "cell 15 | left -44") != NULL);
}

void it_should_shrink_in_larger_steps_to_keep_all_rows_visible(void) {
  const TuringMachine *machine = turing_init("1RB1RB_0LA0LA", false);
  queue_steps(30);
  close_after_first_frame();
  tape_view_run(renderer, machine, false, 0);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "rows 0-30") != NULL);
  assert(strstr(title, "cell 15 | left -33") != NULL);
}

static void queue_ctrl_wheel(float amount) {
  SDL_SetModState(KMOD_CTRL);
  SDL_Event event = {.type = SDL_MOUSEWHEEL};
  event.wheel.preciseY = amount;
  event.wheel.mouseX = 512;
  event.wheel.mouseY = 84;
  SDL_PushEvent(&event);
}

void it_should_zoom_in_around_the_mouse_and_keep_zoom_when_stepping(void) {
  const TuringMachine *machine = turing_init("1RA1RA", false);
  queue_ctrl_wheel(1);
  queue_steps(1);
  close_after_first_frame();
  tape_view_run(renderer, machine, false, 24);
  SDL_SetModState(KMOD_NONE);

  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "cell 25 | left -8") != NULL);
  assert(strstr(title, "HISTORY") != NULL);
  assert(machine->head == 25);
  // Logical cell 12 remains under x=512: -13+500/20 == -8+500/25.
}

void it_should_zoom_out_with_ctrl_wheel_down(void) {
  const TuringMachine *machine = turing_init("1RA1RA", false);
  queue_ctrl_wheel(-1);
  close_after_first_frame();
  tape_view_run(renderer, machine, false, 24);
  SDL_SetModState(KMOD_NONE);

  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "cell 16 | left -19") != NULL);
  assert(machine->head == 24);
}

void it_should_restore_automatic_fitting_with_end(void) {
  const TuringMachine *machine = turing_init("1RA1RA", false);
  queue_ctrl_wheel(1);
  SDL_Event event = {.type = SDL_KEYDOWN};
  event.key.keysym.sym = SDLK_END;
  SDL_PushEvent(&event);
  close_after_first_frame();
  tape_view_run(renderer, machine, false, 24);
  SDL_SetModState(KMOD_NONE);

  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "cell 20 | left -13") != NULL);
  assert(strstr(title, "LIVE") != NULL);
}

void it_should_display_and_restart_a_multi_letter_starting_state(void) {
  char instructions[8000] = "0LA0LA";
  for (int i = 1; i < 999; i++)
    strcat(instructions, "_0LA0LA");
  strcat(instructions, "_1RALM0LALM");
  const TuringMachine *machine = turing_init_with_state(instructions, false, state_from_index(999));
  click_restart();
  close_after_first_frame();
  tape_view_run(renderer, machine, true, 2);

  assert(machine->state == state_from_index(999));
  assert(machine->head == 0);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "state ALM") != NULL);
  assert(strstr(title, "step 0/0") != NULL);
}

void it_should_show_undefined_transitions_as_halted(void) {
  const TuringMachine *machine = turing_init("---0RA", false);
  close_after_first_frame();
  tape_view_run(renderer, machine, false, 1);

  assert(machine->state == 'H');
  assert(machine->head == 0);
  assert(machine->tape[0] == 0);
  const char *title = SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
  assert(strstr(title, "HALTED") != NULL);
  assert(strstr(title, "MISSING TRANSITION") == NULL);
}

int main(void) {
  SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("test", 0, 0, 1024, 768, 0);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  assert(renderer != NULL);

  it_should_open_with_the_preview_steps_already_done();
  it_should_continue_after_the_preview();
  it_should_keep_a_halted_preview_halted();
  it_should_still_start_standalone_machines_at_zero();
  it_should_restart_the_selected_machine_and_clear_history();
  it_should_allow_stepping_after_restarting_a_halted_machine();
  it_should_show_the_head_on_each_row_when_checked();
  it_should_hide_past_heads_when_unchecked();
  it_should_toggle_play_and_pause();
  it_should_keep_earlier_heads_in_place_while_stepping();
  it_should_wait_until_near_the_right_edge_before_fitting();
  it_should_fit_when_near_the_left_edge();
  it_should_shrink_in_larger_steps_to_keep_all_rows_visible();
  it_should_zoom_in_around_the_mouse_and_keep_zoom_when_stepping();
  it_should_zoom_out_with_ctrl_wheel_down();
  it_should_restore_automatic_fitting_with_end();
  it_should_display_and_restart_a_multi_letter_starting_state();
  it_should_show_undefined_transitions_as_halted();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
