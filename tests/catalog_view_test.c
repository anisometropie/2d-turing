#include "catalog_view.h"

#include <assert.h>
#include <string.h>

static SDL_Renderer *renderer;

static void click(int x, int y) {
  SDL_Event event = {.type = SDL_MOUSEBUTTONDOWN};
  event.button.button = SDL_BUTTON_LEFT;
  event.button.x = x;
  event.button.y = y;
  SDL_PushEvent(&event);
}

static void key(SDL_Keycode code) {
  SDL_Event event = {.type = SDL_KEYDOWN};
  event.key.keysym.sym = code;
  SDL_PushEvent(&event);
}

static const char *run_until_quit(unsigned long states) {
  SDL_Event event = {.type = SDL_QUIT};
  SDL_PushEvent(&event);
  assert(catalog_view_run(renderer, states) == 0);
  return SDL_GetWindowTitle(SDL_RenderGetWindow(renderer));
}

void it_should_navigate_a_catalog_larger_than_64_bits(void) {
  key(SDLK_END);
  const char *title = run_until_quit(8);
  assert(strstr(title, "8-state machines") != NULL);
  assert(strstr(title, "page 1.2435E23/1.2435E23") != NULL);
  assert(strstr(title, "1RH1RH_") != NULL);
}

void it_should_jump_by_the_first_transition_in_a_large_catalog(void) {
  click(220, 56);
  const char *title = run_until_quit(26);
  assert(strstr(title, "26-state machines") != NULL);
  assert(strstr(title, "0LB0LA_") != NULL);
}

void it_should_jump_by_the_first_two_transitions_in_a_large_catalog(void) {
  click(198, 56);
  const char *title = run_until_quit(26);
  assert(strstr(title, "0LA0LB_") != NULL);
}

void it_should_accept_a_typed_state_count(void) {
  click(1000, 18);
  SDL_Event event = {.type = SDL_TEXTINPUT};
  strcpy(event.text.text, "1000");
  SDL_PushEvent(&event);
  key(SDLK_RETURN);
  const char *title = run_until_quit(3);
  assert(strstr(title, "1000-state machines") != NULL);
  assert(strstr(title, "page 1/") != NULL);
  assert(strstr(title, "0LA0LA_") != NULL);
}

void it_should_reject_zero_states(void) {
  click(1000, 18);
  SDL_Event event = {.type = SDL_TEXTINPUT};
  strcpy(event.text.text, "0");
  SDL_PushEvent(&event);
  key(SDLK_RETURN);
  const char *title = run_until_quit(3);
  assert(strstr(title, "3-state machines") != NULL);
}

void it_should_increase_and_decrease_the_state_count(void) {
  click(1125, 18);
  const char *title = run_until_quit(26);
  assert(strstr(title, "27-state machines") != NULL);
  click(900, 18);
  title = run_until_quit(26);
  assert(strstr(title, "25-state machines") != NULL);
}

void it_should_open_a_large_catalog_preview_at_its_last_step(void) {
  key(SDLK_END);
  click(40, 90);
  const char *title = run_until_quit(8);
  assert(strstr(title, "step 1/1") != NULL);
  assert(strstr(title, "state H") != NULL);
  assert(strstr(title, "HALTED") != NULL);
}

int main(void) {
  SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("test", 0, 0, 1152, 768, 0);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  assert(renderer != NULL);

  it_should_navigate_a_catalog_larger_than_64_bits();
  it_should_jump_by_the_first_transition_in_a_large_catalog();
  it_should_jump_by_the_first_two_transitions_in_a_large_catalog();
  it_should_accept_a_typed_state_count();
  it_should_reject_zero_states();
  it_should_increase_and_decrease_the_state_count();
  it_should_open_a_large_catalog_preview_at_its_last_step();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
