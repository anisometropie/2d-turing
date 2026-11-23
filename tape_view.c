#include "tape_view.h"
#include "ui_text.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

enum { MAX_CELL_SIZE = 20, MARGIN = 12, TOOLBAR_HEIGHT = 84, MAX_SPEED = 10000 };

typedef struct {
  int64_t head;
  int64_t written_position;
  unsigned char previous_symbol;
  char state;
} TapeRow;

typedef struct {
  TapeRow *rows;
  size_t count;
  size_t capacity;
  int64_t minimum_head;
  int64_t maximum_head;
} TapeHistory;

typedef struct {
  SDL_Texture *texture;
  Uint32 *pixels;
  unsigned char *cells;
  int columns;
  int rows;
} TapeCanvas;

typedef struct {
  size_t first_row;
  int64_t left_cell;
  int cell_size;
  bool follow_latest;
  double horizontal_remainder;
  double vertical_remainder;
} TapeViewport;

static bool reserve_row(TapeHistory *history) {
  if (history->count < history->capacity)
    return true;
  if (history->capacity > SIZE_MAX / sizeof(TapeRow) / 2)
    return false;
  size_t capacity = history->capacity ? history->capacity * 2 : 64;
  TapeRow *rows = realloc(history->rows, capacity * sizeof *rows);
  if (rows == NULL)
    return false;
  history->rows = rows;
  history->capacity = capacity;
  return true;
}

static const char *advance(TapeHistory *history, const TuringMachine *machine) {
  if (machine->state == 'H')
    return "HALTED";

  int state = machine->state - 'A';
  unsigned char symbol = machine->tape[machine->head];
  size_t offset = (size_t)state * 7 + symbol * 3;
  if (state < 0 || state > 25 || symbol > 1 || offset + 2 >= strlen(machine->instructions))
    return "MISSING TRANSITION";
  if (!reserve_row(history))
    return "OUT OF MEMORY";

  int64_t written_position = machine->head;
  StepResult result = step();
  history->rows[history->count++] = (TapeRow) {
    .head = machine->head,
    .written_position = written_position,
    .previous_symbol = symbol,
    .state = machine->state
  };
  if (machine->head < history->minimum_head)
    history->minimum_head = machine->head;
  if (machine->head > history->maximum_head)
    history->maximum_head = machine->head;
  return result == STEP_HALTED ? "HALTED" : NULL;
}

static void free_canvas(TapeCanvas *canvas) {
  SDL_DestroyTexture(canvas->texture);
  free(canvas->pixels);
  free(canvas->cells);
  *canvas = (TapeCanvas){0};
}

static bool resize_canvas(SDL_Renderer *renderer, TapeCanvas *canvas, int columns, int rows) {
  if (columns == canvas->columns && rows == canvas->rows)
    return true;
  free_canvas(canvas);
  canvas->pixels = malloc((size_t)columns * rows * sizeof *canvas->pixels);
  canvas->cells = malloc((size_t)columns);
  canvas->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING, columns, rows);
  if (!canvas->pixels || !canvas->cells || !canvas->texture)
    return false;
  SDL_SetTextureScaleMode(canvas->texture, SDL_ScaleModeNearest);
  canvas->columns = columns;
  canvas->rows = rows;
  return true;
}

static SDL_Rect speed_track(int width) {
  return (SDL_Rect){MARGIN, 40, width - 2 * MARGIN, 6};
}

static SDL_Rect step_button(int width) {
  return (SDL_Rect){width - MARGIN - 68, 6, 68, 24};
}

static SDL_Rect back_button(void) {
  return (SDL_Rect){MARGIN + 36, 50, 88, 16};
}

static double slider_position(int x, int width) {
  SDL_Rect track = speed_track(width);
  double position = (double)(x - track.x) / track.w;
  return fmax(0.0, fmin(1.0, position));
}

static void undo_write(unsigned char *cells, int columns, int64_t left, const TapeRow *row) {
  int64_t column = row->written_position - left;
  if (column >= 0 && column < columns)
    cells[column] = row->previous_symbol;
}

static void update_viewport(SDL_Renderer *renderer, const TapeHistory *history,
                            TapeViewport *view) {
  int width;
  int height;
  SDL_GetRendererOutputSize(renderer, &width, &height);
  int usable_width = width - 2 * MARGIN;
  int usable_height = height - TOOLBAR_HEIGHT - MARGIN;
  if (usable_width < 1 || usable_height < 1)
    return;

  if (view->follow_latest) {
    int size = MAX_CELL_SIZE;
    size_t span = (size_t)(history->maximum_head - history->minimum_head) + 9;
    if (history->count > (size_t)(usable_height / size))
      size = (int)((size_t)usable_height / history->count);
    if (size < 1)
      size = 1;
    if (span > (size_t)(usable_width / size))
      size = (int)((size_t)usable_width / span);
    view->cell_size = size < 1 ? 1 : size;

    int columns = usable_width / view->cell_size;
    int64_t center = history->minimum_head + (history->maximum_head - history->minimum_head) / 2;
    if (span > (size_t)columns)
      center = history->rows[history->count - 1].head;
    view->left_cell = center - columns / 2;
  }

  size_t visible = (size_t)(usable_height / view->cell_size);
  if (visible < 1)
    visible = 1;
  size_t maximum = history->count > visible ? history->count - visible : 0;
  if (view->follow_latest || view->first_row > maximum)
    view->first_row = maximum;
}

static bool scroll_viewport(SDL_Renderer *renderer, const TapeHistory *history,
                             TapeViewport *view, double horizontal, double vertical) {
  update_viewport(renderer, history, view);
  view->horizontal_remainder += horizontal * 48.0 / view->cell_size;
  view->vertical_remainder += vertical * 48.0 / view->cell_size;
  int64_t dx = (int64_t)view->horizontal_remainder;
  int64_t dy = (int64_t)view->vertical_remainder;
  view->horizontal_remainder -= dx;
  view->vertical_remainder -= dy;

  int height;
  SDL_GetRendererOutputSize(renderer, NULL, &height);
  int rows = (height - TOOLBAR_HEIGHT - MARGIN) / view->cell_size;
  size_t visible = rows > 0 ? (size_t)rows : 1;
  size_t maximum = history->count > visible ? history->count - visible : 0;
  size_t previous = view->first_row;
  if (dy > 0) {
    size_t amount = (size_t)dy;
    view->first_row -= amount < view->first_row ? amount : view->first_row;
  } else if (dy < 0) {
    size_t amount = (size_t)-dy;
    size_t remaining = maximum - view->first_row;
    view->first_row += amount < remaining ? amount : remaining;
  }
  if (view->first_row != previous)
    view->follow_latest = view->first_row == maximum;
  if (dx != 0) {
    view->left_cell += dx;
    view->follow_latest = false;
  }
  return view->first_row != previous || dx != 0;
}

static bool draw_history(SDL_Renderer *renderer, const TuringMachine *machine,
                         const TapeHistory *history, TapeCanvas *canvas,
                         TapeViewport *view, bool paused, const char *stopped,
                         double slider, int speed, bool from_grid) {
  update_viewport(renderer, history, view);
  int width;
  int height;
  SDL_GetRendererOutputSize(renderer, &width, &height);
  int usable_width = width - 2 * MARGIN;
  int usable_height = height - TOOLBAR_HEIGHT - MARGIN;
  if (usable_width < 1 || usable_height < 1)
    return true;

  int cell_size = view->cell_size;
  int columns = usable_width / cell_size;
  int visible_rows = usable_height / cell_size;
  size_t first = view->first_row;
  size_t rows_remaining = history->count - first;
  size_t end = first + (rows_remaining < (size_t)visible_rows ? rows_remaining : (size_t)visible_rows);
  const TapeRow *selected = &history->rows[end - 1];
  int64_t left = view->left_cell;
  int64_t origin = machine->tape - machine->buffer;
  int rows = (int)(end - first);
  if (!resize_canvas(renderer, canvas, columns, rows))
    return false;

  // Start with the live tape and undo writes to recover each earlier row.
  for (int column = 0; column < columns; column++) {
    int64_t index = origin + left + column;
    canvas->cells[column] = index >= 0 && index < (int64_t)machine->capacity
      ? machine->buffer[index] : machine->blank_symbol;
  }
  for (size_t row = history->count; row > end;) {
    row--;
    undo_write(canvas->cells, columns, left, &history->rows[row]);
  }
  for (size_t row = end; row > first;) {
    row--;
    for (int column = 0; column < columns; column++) {
      canvas->pixels[(row - first) * columns + column] =
        canvas->cells[column] ? 0xff000000 : 0xffffffff;
    }
    if (row > 0)
      undo_write(canvas->cells, columns, left, &history->rows[row]);
  }

  SDL_SetRenderDrawColor(renderer, 210, 210, 210, 255);
  SDL_RenderClear(renderer);
  SDL_UpdateTexture(canvas->texture, NULL, canvas->pixels, columns * (int)sizeof *canvas->pixels);
  SDL_Rect destination = {MARGIN, TOOLBAR_HEIGHT, columns * cell_size, rows * cell_size};
  SDL_RenderCopy(renderer, canvas->texture, NULL, &destination);
  if (cell_size >= 4) {
    SDL_SetRenderDrawColor(renderer, 205, 205, 205, 255);
    for (int column = 1; column < columns; column++) {
      int x = MARGIN + column * cell_size - 1;
      SDL_RenderDrawLine(renderer, x, TOOLBAR_HEIGHT, x, TOOLBAR_HEIGHT + destination.h - 1);
    }
    for (int row = 1; row < rows; row++) {
      int y = TOOLBAR_HEIGHT + row * cell_size - 1;
      SDL_RenderDrawLine(renderer, MARGIN, y, MARGIN + destination.w - 1, y);
    }
  }
  int64_t head_column = selected->head - left;
  if (head_column >= 0 && head_column < columns) {
    SDL_Rect head = {MARGIN + (int)head_column * cell_size,
                     TOOLBAR_HEIGHT + (rows - 1) * cell_size, cell_size, cell_size};
    SDL_SetRenderDrawColor(renderer, 220, 65, 35, 255);
    SDL_RenderDrawRect(renderer, &head);
  }

  SDL_Rect toolbar = {0, 0, width, TOOLBAR_HEIGHT};
  SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
  SDL_RenderFillRect(renderer, &toolbar);
  SDL_Rect track = speed_track(width);
  SDL_SetRenderDrawColor(renderer, 175, 175, 175, 255);
  SDL_RenderFillRect(renderer, &track);
  SDL_Rect progress = track;
  progress.w = (int)(slider * track.w);
  SDL_SetRenderDrawColor(renderer, 50, 100, 180, 255);
  SDL_RenderFillRect(renderer, &progress);
  SDL_Rect thumb = {track.x + progress.w - 5, track.y - 5, 10, 16};
  SDL_RenderFillRect(renderer, &thumb);

  SDL_Rect button = step_button(width);
  SDL_SetRenderDrawColor(renderer, stopped ? 220 : 50, stopped ? 220 : 100,
                        stopped ? 220 : 180, 255);
  SDL_RenderFillRect(renderer, &button);
  SDL_SetRenderDrawColor(renderer, stopped ? 140 : 255, stopped ? 140 : 255,
                        stopped ? 140 : 255, 255);
  ui_text(renderer, button.x + 10, button.y + 5, "STEP", 2);

  char label[256];
  SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
  snprintf(label, sizeof label, "SPEED %d STEPS/S", speed);
  ui_text(renderer, MARGIN, 12, label, 2);
  ui_text(renderer, MARGIN, 54, "1", 1);
  ui_text(renderer, width - MARGIN - 30, 54, "10000", 1);
  if (from_grid) {
    SDL_Rect back = back_button();
    SDL_SetRenderDrawColor(renderer, 50, 100, 180, 255);
    SDL_RenderFillRect(renderer, &back);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    ui_text(renderer, back.x + 10, back.y + 4, "BACK TO GRID", 1);
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
  }
  const char *status = stopped ? stopped : paused ? "PAUSED" : "RUNNING";
  int status_x = button.x - 12 - (int)strlen(status) * 12;
  if (status_x > MARGIN + (int)strlen(label) * 12 + 12)
    ui_text(renderer, status_x, 12, status, 2);
  snprintf(label, sizeof label, "ROWS %zu-%zu/%zu  STATE %c  HEAD %" PRId64 "  CELL %d  %s",
           first, end - 1, history->count - 1, selected->state, selected->head,
           cell_size, view->follow_latest ? "LIVE" : "HISTORY");
  ui_text(renderer, MARGIN, 68, label, 1);
  SDL_RenderPresent(renderer);

  SDL_Window *window = SDL_RenderGetWindow(renderer);
  if (window != NULL) {
    snprintf(label, sizeof label,
             "Tape %s | rows %zu-%zu | step %zu/%zu | state %c | head %" PRId64
             " | %s | %d steps/s | cell %d | left %" PRId64 " | %s | End: live",
             machine->instructions, first, end - 1, end - 1, history->count - 1, selected->state, selected->head,
             status, speed, cell_size, left, view->follow_latest ? "LIVE" : "HISTORY");
    SDL_SetWindowTitle(window, label);
  }
  return true;
}

TapeViewResult tape_view_run(SDL_Renderer *renderer, const TuringMachine *machine,
                             bool from_grid, size_t initial_steps) {
  TapeHistory history = {0};
  TapeCanvas canvas = {0};
  TapeViewport view = {.cell_size = MAX_CELL_SIZE, .follow_latest = true};
  if (!reserve_row(&history))
    return TAPE_VIEW_ERROR;
  history.rows[history.count++] = (TapeRow){.head = machine->head, .state = machine->state};
  history.minimum_head = history.maximum_head = machine->head;
  SDL_Window *window = SDL_RenderGetWindow(renderer);
  if (window != NULL)
    SDL_SetWindowMinimumSize(window, 420, 220);

  bool running = true;
  bool paused = true;
  bool dirty = true;
  bool dragging = false;
  const char *stopped = machine->state == 'H' ? "HALTED" : NULL;
  for (size_t i = 0; i < initial_steps && stopped == NULL; i++)
    stopped = advance(&history, machine);
  double slider = log(60.0) / log(MAX_SPEED);
  int speed = 60;
  double credit = 0;
  Uint64 previous_frame = SDL_GetPerformanceCounter();
  double frequency = (double)SDL_GetPerformanceFrequency();
  TapeViewResult result = TAPE_VIEW_EXIT;

  while (running) {
    SDL_Event event;
    while (running && SDL_PollEvent(&event)) {
      bool step_requested = false;
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
          running = false;
          result = TAPE_VIEW_BACK;
          break;
        case SDLK_SPACE:
          paused = !paused;
          credit = 0;
          dirty = true;
          break;
        case SDLK_RIGHT:
          step_requested = true;
          break;
        case SDLK_HOME: {
          update_viewport(renderer, &history, &view);
          int width;
          SDL_GetRendererOutputSize(renderer, &width, NULL);
          view.first_row = 0;
          view.left_cell = history.rows[0].head - (width - 2 * MARGIN) / view.cell_size / 2;
          view.follow_latest = false;
          view.horizontal_remainder = view.vertical_remainder = 0;
          dirty = true;
          break;
        }
        case SDLK_END:
          view.follow_latest = true;
          view.horizontal_remainder = view.vertical_remainder = 0;
          dirty = true;
          break;
        }
      } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int width;
        SDL_GetRendererOutputSize(renderer, &width, NULL);
        SDL_Rect button = step_button(width);
        SDL_Point mouse = {event.button.x, event.button.y};
        SDL_Rect back = back_button();
        if (from_grid && SDL_PointInRect(&mouse, &back)) {
          result = TAPE_VIEW_BACK;
          running = false;
          break;
        }
        step_requested = stopped == NULL && SDL_PointInRect(&mouse, &button);
        dragging = !step_requested && event.button.y >= 30 && event.button.y <= 56;
        if (dragging) {
          slider = slider_position(event.button.x, width);
          speed = (int)(pow(MAX_SPEED, slider) + 0.5);
          credit = 0;
          dirty = true;
        }
      } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        dragging = false;
      } else if (event.type == SDL_MOUSEMOTION && dragging) {
        int width;
        SDL_GetRendererOutputSize(renderer, &width, NULL);
        slider = slider_position(event.motion.x, width);
        speed = (int)(pow(MAX_SPEED, slider) + 0.5);
        credit = 0;
        dirty = true;
      } else if (event.type == SDL_MOUSEWHEEL) {
        double horizontal = event.wheel.preciseX;
        double vertical = event.wheel.preciseY;
        if (horizontal == 0)
          horizontal = event.wheel.x;
        if (vertical == 0)
          vertical = event.wheel.y;
        if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
          horizontal = -horizontal;
          vertical = -vertical;
        }
        if (SDL_GetModState() & KMOD_SHIFT) {
          horizontal -= vertical;
          vertical = 0;
        }
        if (scroll_viewport(renderer, &history, &view, horizontal, vertical))
          dirty = true;
      } else if (event.type == SDL_WINDOWEVENT) {
        dirty = true;
      }
      if (step_requested) {
        paused = true;
        credit = 0;
        if (stopped == NULL)
          stopped = advance(&history, machine);
        view.follow_latest = true;
        view.horizontal_remainder = view.vertical_remainder = 0;
        dirty = true;
      }
    }

    Uint64 now = SDL_GetPerformanceCounter();
    double elapsed = (now - previous_frame) / frequency;
    previous_frame = now;
    size_t count = history.count;
    const char *previous_status = stopped;
    if (running && !paused && stopped == NULL) {
      credit = fmin(credit + fmin(elapsed, 0.25) * speed, fmax(1.0, speed * 0.25));
      while (credit >= 1 && stopped == NULL) {
        stopped = advance(&history, machine);
        credit -= 1;
        if ((history.count - count) % 64 == 0 &&
            (SDL_GetPerformanceCounter() - now) / frequency >= 0.008)
          break;
      }
    } else {
      credit = 0;
    }
    if (history.count != count || stopped != previous_status) {
      dirty = true;
    }
    if (dirty) {
      if (!draw_history(renderer, machine, &history, &canvas,
                        &view, paused, stopped, slider, speed, from_grid)) {
        fprintf(stderr, "Could not render tape history: %s\n", SDL_GetError());
        result = TAPE_VIEW_ERROR;
        break;
      }
      dirty = false;
    }
    SDL_Delay(16);
  }

  free_canvas(&canvas);
  free(history.rows);
  return result;
}
