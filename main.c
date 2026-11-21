#include "window.h"

enum { WINDOW_WIDTH = 1024, WINDOW_HEIGHT = 768, SQUARE_SIZE = 120 };

static void draw_square(SDL_Renderer *renderer) {
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
