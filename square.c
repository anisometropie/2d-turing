#include "square.h"

enum { SQUARE_SIZE = 120 };

void draw_square(SDL_Renderer *renderer) {
  int width;
  int height;
  SDL_GetRendererOutputSize(renderer, &width, &height);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);

  SDL_Rect square = {
    (width - SQUARE_SIZE) / 2,
    (height - SQUARE_SIZE) / 2,
    SQUARE_SIZE,
    SQUARE_SIZE
  };
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderFillRect(renderer, &square);
  SDL_RenderPresent(renderer);
}
