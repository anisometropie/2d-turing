#include "window.h"
#include "tape_view.h"
#include "catalog_view.h"

#include <string.h>

enum { WINDOW_WIDTH = 1024, WINDOW_HEIGHT = 768 };

int main(int argc, char **argv) {
  SDL_Renderer *renderer = window_open("2d-turing", WINDOW_WIDTH, WINDOW_HEIGHT);
  if (renderer == NULL)
    return 1;

  int result;
  if (argc > 1 && strcmp(argv[1], "--one-state") == 0) {
    result = catalog_view_run(renderer, 1);
  } else if (argc > 1 && strcmp(argv[1], "--two-state") == 0) {
    result = catalog_view_run(renderer, 2);
  } else {
    const char *program = argc > 1 ? argv[1] : "1RB1RH_1LB0RC_1LC1LA";
    const TuringMachine *machine = turing_init(program, false);
    result = tape_view_run(renderer, machine, false, 0) == TAPE_VIEW_ERROR ? 1 : 0;
  }
  window_close(renderer);
  return result;
}
