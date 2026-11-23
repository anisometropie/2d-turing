#include "catalog_view.h"
#include "machine_catalog.h"
#include "tape_view.h"
#include "ui_text.h"

#include <stdio.h>

enum { GRID_SIDE = 8, MARGIN = 12, GAP = 6, HEADER_HEIGHT = 72, PAGE_SIZE = 64 };

static SDL_Rect tile_rect(int index, int width, int height) {
  int tile_width = (width - 2 * MARGIN - 7 * GAP) / GRID_SIDE;
  int tile_height = (height - HEADER_HEIGHT - MARGIN - 7 * GAP) / GRID_SIDE;
  return (SDL_Rect){
    MARGIN + (index % GRID_SIDE) * (tile_width + GAP),
    HEADER_HEIGHT + (index / GRID_SIDE) * (tile_height + GAP),
    tile_width, tile_height
  };
}

static void draw_tile(SDL_Renderer *renderer, const MachinePreview *preview,
                      int index, SDL_Rect tile) {
  SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
  SDL_RenderFillRect(renderer, &tile);
  SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
  SDL_RenderDrawRect(renderer, &tile);

  char label[32];
  snprintf(label, sizeof label, "%s", preview->instructions);
  SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
  ui_text(renderer, tile.x + 6, tile.y + 5, label, 1);

  int cell_size = (tile.w - 12) / PREVIEW_COLUMNS;
  int vertical_size = (tile.h - 30) / (PREVIEW_STEPS + 1);
  if (vertical_size < cell_size)
    cell_size = vertical_size;
  if (cell_size < 1)
    cell_size = 1;
  int left = tile.x + (tile.w - PREVIEW_COLUMNS * cell_size) / 2;
  int top = tile.y + 17;
  SDL_Rect tape = {left, top, PREVIEW_COLUMNS * cell_size,
                   (preview->steps + 1) * cell_size};
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderFillRect(renderer, &tape);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  for (int row = 0; row <= preview->steps; row++) {
    for (int column = 0; column < PREVIEW_COLUMNS; column++) {
      if (preview->cells[row][column]) {
        SDL_Rect cell = {left + column * cell_size, top + row * cell_size,
                         cell_size, cell_size};
        SDL_RenderFillRect(renderer, &cell);
      }
    }
  }
  // Show the head on each row, including machines that only write zeros.
  SDL_SetRenderDrawColor(renderer, 220, 65, 35, 255);
  for (int row = 0; row <= preview->steps; row++) {
    int column = PREVIEW_STEPS + preview->heads[row];
    SDL_Rect head = {left + column * cell_size, top + row * cell_size,
                     cell_size, cell_size};
    SDL_RenderDrawRect(renderer, &head);
  }
  SDL_SetRenderDrawColor(renderer, preview->halted ? 150 : 30,
                        preview->halted ? 65 : 90, 30, 255);
  snprintf(label, sizeof label, "%d %s %d", index + 1,
           preview->halted ? "HALT" : "STEP", preview->steps);
  ui_text(renderer, tile.x + 6, tile.y + tile.h - 11, label, 1);
}

static SDL_Rect navigation_button(int index, int width) {
  if (index < 4)
    return (SDL_Rect){MARGIN + index * 76, 46, 68, 20};
  return (SDL_Rect){width - MARGIN - (6 - index) * 92, 6, 84, 24};
}

static void draw_grid(SDL_Renderer *renderer, const MachinePreview *previews,
                       int states, int page) {
  int width;
  int height;
  SDL_GetRendererOutputSize(renderer, &width, &height);
  int count = machine_catalog_count(states);
  int pages = count / PAGE_SIZE;
  SDL_SetRenderDrawColor(renderer, 225, 225, 225, 255);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
  char label[96];
  snprintf(label, sizeof label, "%d-STATE MACHINES / %d TOTAL", states, count);
  ui_text(renderer, MARGIN, 12, label, 2);
  ui_text(renderer, MARGIN, 34, "ZERO TAPE / FIRST 24 STEPS / RED: HEAD / CLICK TO OPEN", 1);
  snprintf(label, sizeof label, "PAGE %d/%d / MACHINES %d-%d / PGUP PGDN HOME END",
           page + 1, pages, page * PAGE_SIZE + 1, (page + 1) * PAGE_SIZE);
  ui_text(renderer, 330, 52, label, 1);

  static const char *labels[] = {"FIRST", "PREV", "NEXT", "LAST", "1 STATE", "2 STATES"};
  for (int i = 0; i < 6; i++) {
    bool enabled = i < 2 ? page > 0 : i < 4 ? page < pages - 1 : states != i - 3;
    SDL_Rect button = navigation_button(i, width);
    SDL_SetRenderDrawColor(renderer, enabled ? 50 : 190, enabled ? 100 : 190,
                          enabled ? 180 : 190, 255);
    SDL_RenderFillRect(renderer, &button);
    SDL_SetRenderDrawColor(renderer, enabled ? 255 : 100, enabled ? 255 : 100,
                          enabled ? 255 : 100, 255);
    ui_text(renderer, button.x + 10, button.y + (button.h - 7) / 2, labels[i], 1);
  }
  for (int i = 0; i < PAGE_SIZE; i++)
    draw_tile(renderer, &previews[i], page * PAGE_SIZE + i, tile_rect(i, width, height));
  SDL_RenderPresent(renderer);
}

int catalog_view_run(SDL_Renderer *renderer, int states) {
  MachinePreview previews[PAGE_SIZE];
  int page = 0;
  bool reload = true;
  bool dirty = true;
  SDL_Window *window = SDL_RenderGetWindow(renderer);
  for (;;) {
    if (reload) {
      for (int i = 0; i < PAGE_SIZE; i++)
        machine_catalog_preview(states, page * PAGE_SIZE + i, &previews[i]);
      reload = false;
      dirty = true;
    }
    int pages = machine_catalog_count(states) / PAGE_SIZE;
    if (dirty) {
      SDL_SetWindowMinimumSize(window, 1024, 768);
      char title[96];
      snprintf(title, sizeof title, "All %d %s-state machines | page %d/%d | click to inspect",
               machine_catalog_count(states), states == 1 ? "one" : "two", page + 1, pages);
      SDL_SetWindowTitle(window, title);
      draw_grid(renderer, previews, states, page);
      dirty = false;
    }

    SDL_Event event;
    if (!SDL_WaitEvent(&event)) {
      fprintf(stderr, "Could not read grid event: %s\n", SDL_GetError());
      return 1;
    }
    if (event.type == SDL_QUIT ||
        (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
      return 0;
    if (event.type == SDL_WINDOWEVENT)
      dirty = true;

    int next_page = page;
    int next_states = states;
    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
      case SDLK_PAGEUP: case SDLK_LEFT: next_page--; break;
      case SDLK_PAGEDOWN: case SDLK_RIGHT: next_page++; break;
      case SDLK_HOME: next_page = 0; break;
      case SDLK_END: next_page = pages - 1; break;
      }
    }
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
      SDL_Point mouse = {event.button.x, event.button.y};
      int width;
      int height;
      SDL_GetRendererOutputSize(renderer, &width, &height);
      for (int i = 0; i < 6; i++) {
        SDL_Rect button = navigation_button(i, width);
        if (!SDL_PointInRect(&mouse, &button))
          continue;
        switch (i) {
        case 0: next_page = 0; break;
        case 1: next_page--; break;
        case 2: next_page++; break;
        case 3: next_page = pages - 1; break;
        case 4: next_states = 1; break;
        case 5: next_states = 2; break;
        }
      }
      for (int i = 0; i < PAGE_SIZE; i++) {
        SDL_Rect tile = tile_rect(i, width, height);
        if (SDL_PointInRect(&mouse, &tile)) {
          const TuringMachine *machine = turing_init(previews[i].instructions, false);
          TapeViewResult result = tape_view_run(renderer, machine, true, previews[i].steps);
          if (result != TAPE_VIEW_BACK)
            return result == TAPE_VIEW_ERROR ? 1 : 0;
          dirty = true;
          break;
        }
      }
    }
    if (next_states != states) {
      states = next_states;
      page = 0;
      reload = true;
    } else {
      if (next_page < 0)
        next_page = 0;
      if (next_page >= pages)
        next_page = pages - 1;
      if (next_page != page) {
        page = next_page;
        reload = true;
      }
    }
  }
}
