#ifndef TAPE_VIEW_H
#define TAPE_VIEW_H

#include "turing.h"
#include <SDL.h>

typedef enum {
  TAPE_VIEW_EXIT,
  TAPE_VIEW_ERROR,
  TAPE_VIEW_BACK
} TapeViewResult;

TapeViewResult tape_view_run(SDL_Renderer *renderer, const TuringMachine *machine,
                             bool from_grid, size_t initial_steps);

#endif
