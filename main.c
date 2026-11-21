#include "window.h"
#include "square.h"

enum { WINDOW_WIDTH = 1024, WINDOW_HEIGHT = 768 };

int main(void) {
  SDL_Renderer *renderer = window_open("2d-turing", WINDOW_WIDTH, WINDOW_HEIGHT);
  if (renderer == NULL)
    return 1;

  draw_square(renderer);
  int result;
  while ((result = window_wait_for_redraw()) > 0)
    draw_square(renderer);

  window_close(renderer);
  return result < 0 ? 1 : 0;
}
