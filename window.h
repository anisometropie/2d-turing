#ifndef WINDOW_H
#define WINDOW_H

#include <SDL.h>

SDL_Renderer *window_open(const char *title, int width, int height);
int window_wait_for_redraw(void);
void window_close(SDL_Renderer *renderer);

#endif
