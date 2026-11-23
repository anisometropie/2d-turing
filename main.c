#include "window.h"
#include "tape_view.h"

enum { WINDOW_WIDTH = 1024, WINDOW_HEIGHT = 768 };

int main(int argc, char **argv) {
  SDL_Renderer *renderer = window_open("2d-turing", WINDOW_WIDTH, WINDOW_HEIGHT);
  if (renderer == NULL)
    return 1;

  const char *program = argc > 1 ? argv[1] : "1RB1RH_1LB0RC_1LC1LA";
  const TuringMachine *machine = turing_init(program, false);
  int result = tape_view_run(renderer, machine);
  window_close(renderer);
  return result;
}
