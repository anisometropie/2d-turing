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
  event.button.x = 180;
  event.button.y = 58;
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

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
