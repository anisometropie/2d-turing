#ifndef TAPE_VIEW_H
#define TAPE_VIEW_H

#include "turing.h"
#include <SDL.h>

int tape_view_run(SDL_Renderer *renderer, const TuringMachine *machine);

#endif
