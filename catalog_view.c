#include "catalog_view.h"
#include "machine_catalog.h"
#include "tape_view.h"
#include "ui_text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Keep navigation exact; abbreviate only the text drawn on screen.
static void number_label(const mpz_t number, char label[32]) {
  char *digits = mpz_get_str(NULL, 10, number);
  size_t length = strlen(digits);
  if (length <= 12)
    snprintf(label, 32, "%s", digits);
  else
    snprintf(label, 32, "%.1s.%.4sE%zu", digits, digits + 1, length - 1);
  free(digits);
}

static void draw_tile(SDL_Renderer *renderer, const MachinePreview *preview,
                      const mpz_t index, SDL_Rect tile) {
  SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
  SDL_RenderFillRect(renderer, &tile);
  SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
  SDL_RenderDrawRect(renderer, &tile);

  SDL_RenderSetClipRect(renderer, &tile);
  char label[128];
  int visible = (tile.w - 12) / 6;
  if (visible > (int)sizeof label - 1)
    visible = sizeof label - 1;
  snprintf(label, sizeof label, "%.*s", visible, preview->instructions);
  if (strlen(preview->instructions) > (size_t)visible && visible >= 3)
    memcpy(label + visible - 3, "...", 3);
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
  char number[32];
  number_label(index, number);
  snprintf(label, sizeof label, "%s %s %d", number,
           preview->halted ? "HALT" : "STEP", preview->steps);
  ui_text(renderer, tile.x + 6, tile.y + tile.h - 11, label, 1);
  SDL_RenderSetClipRect(renderer, NULL);
}

static SDL_Rect navigation_button(int index) {
  if (index == 1 || index == 8 || index == 9) {
    int part = index == 1 ? 2 : index == 9 ? 1 : 0;
    return (SDL_Rect){MARGIN + 76 + part * 23, 46, part == 2 ? 22 : 21, 20};
  }
  if (index == 2 || index == 6 || index == 7) {
    int part = index == 2 ? 0 : index == 7 ? 1 : 2;
    return (SDL_Rect){MARGIN + 2 * 76 + part * 23, 46, part == 2 ? 22 : 21, 20};
  }
  return (SDL_Rect){MARGIN + index * 76, 46, 68, 20};
}

static SDL_Rect state_control(int part, int width) {
  int left = width - MARGIN - 254;
  if (part == 0)
    return (SDL_Rect){left, 6, 24, 24};
  if (part == 1)
    return (SDL_Rect){left + 30, 6, 194, 24};
  return (SDL_Rect){left + 230, 6, 24, 24};
}

static bool group_target(unsigned long states, const mpz_t selected, int button, mpz_t target) {
  int symbols = button == 6 || button == 8 ? 3 : 6;
  int direction = button < 8 ? 1 : -1;
  return machine_catalog_jump(states, selected, symbols, direction, target);
}

static void draw_grid(SDL_Renderer *renderer, const MachinePreview *previews,
                       unsigned long states, const mpz_t count, const mpz_t pages,
                       const mpz_t page, const mpz_t selected,
                       const char *input, bool editing, bool invalid) {
  int width;
  int height;
  SDL_GetRendererOutputSize(renderer, &width, &height);
  SDL_SetRenderDrawColor(renderer, 225, 225, 225, 255);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
  char label[192], total[32], page_number[32], page_count[32];
  mpz_t index, target;
  mpz_inits(index, target, NULL);
  number_label(count, total);
  snprintf(label, sizeof label, "%lu STATES / %s MACHINES", states, total);
  SDL_Rect heading = {MARGIN, 6, width - 290, 24};
  SDL_RenderSetClipRect(renderer, &heading);
  ui_text(renderer, MARGIN, 12, label, 2);
  SDL_RenderSetClipRect(renderer, NULL);
  ui_text(renderer, MARGIN, 34,
          invalid ? "ENTER A POSITIVE STATE COUNT" : editing ? "TYPE STATE COUNT / ENTER TO APPLY / ESC TO CANCEL" :
          "ZERO TAPE / FIRST 24 STEPS / RED: HEAD / BLUE: SELECTED / CLICK TO OPEN", 1);
  mpz_add_ui(index, page, 1);
  number_label(index, page_number);
  number_label(pages, page_count);
  snprintf(label, sizeof label, "PAGE %s/%s / > PAGE  >> 6 SYMBOLS  >>> 3 SYMBOLS",
           page_number, page_count);
  ui_text(renderer, 330, 52, label, 1);

  static const char *labels[] = {"FIRST", "<", ">", "LAST", "", "", ">>>", ">>", "<<<", "<<"};
  for (int i = 0; i < 10; i++) {
    if (i == 4 || i == 5)
      continue;
    bool enabled = i == 0 ? mpz_sgn(selected) > 0 : i == 1 ? mpz_sgn(page) > 0 :
      i < 4 ? mpz_cmp(index, pages) < 0 : group_target(states, selected, i, target);
    SDL_Rect button = navigation_button(i);
    SDL_SetRenderDrawColor(renderer, enabled ? 50 : 190, enabled ? 100 : 190,
                          enabled ? 180 : 190, 255);
    SDL_RenderFillRect(renderer, &button);
    SDL_SetRenderDrawColor(renderer, enabled ? 255 : 100, enabled ? 255 : 100,
                          enabled ? 255 : 100, 255);
    int label_width = (int)strlen(labels[i]) * 6;
    ui_text(renderer, button.x + (button.w - label_width) / 2,
            button.y + (button.h - 7) / 2, labels[i], 1);
  }
  for (int part = 0; part < 3; part++) {
    SDL_Rect control = state_control(part, width);
    SDL_SetRenderDrawColor(renderer, part == 1 ? 255 : 50, part == 1 ? 255 : 100,
                          part == 1 ? 255 : 180, 255);
    SDL_RenderFillRect(renderer, &control);
    SDL_SetRenderDrawColor(renderer, editing ? 50 : 150, editing ? 100 : 150,
                          editing ? 180 : 150, 255);
    SDL_RenderDrawRect(renderer, &control);
    SDL_SetRenderDrawColor(renderer, part == 1 ? 30 : 255, part == 1 ? 30 : 255,
                          part == 1 ? 30 : 255, 255);
    snprintf(label, sizeof label, "STATES %s", input);
    ui_text(renderer, control.x + 9, control.y + 8, part == 1 ? label : part == 0 ? "-" : "+", 1);
  }
  mpz_mul_ui(index, page, PAGE_SIZE);
  unsigned long selected_slot = mpz_fdiv_ui(selected, PAGE_SIZE);
  for (int i = 0; i < PAGE_SIZE; i++) {
    SDL_Rect tile = tile_rect(i, width, height);
    mpz_add_ui(index, index, 1);
    draw_tile(renderer, &previews[i], index, tile);
    if ((unsigned long)i == selected_slot) {
      SDL_SetRenderDrawColor(renderer, 50, 100, 180, 255);
      SDL_RenderDrawRect(renderer, &tile);
      tile.x++;
      tile.y++;
      tile.w -= 2;
      tile.h -= 2;
      SDL_RenderDrawRect(renderer, &tile);
    }
  }
  mpz_clears(index, target, NULL);
  SDL_RenderPresent(renderer);
}

int catalog_view_run(SDL_Renderer *renderer, unsigned long states) {
  MachinePreview previews[PAGE_SIZE] = {0};
  mpz_t count, pages, page, selected, next_page, next_selected, index;
  mpz_inits(count, pages, page, selected, next_page, next_selected, index, NULL);
  int result = 0;
  if (!machine_catalog_count(states, count)) {
    result = 1;
    goto cleanup;
  }
  mpz_divexact_ui(pages, count, PAGE_SIZE);
  bool reload = true;
  bool dirty = true;
  bool editing = false;
  bool replace_input = false;
  bool invalid = false;
  char input[32];
  snprintf(input, sizeof input, "%lu", states);
  SDL_Window *window = SDL_RenderGetWindow(renderer);
  for (;;) {
    if (reload) {
      mpz_mul_ui(index, page, PAGE_SIZE);
      for (int i = 0; i < PAGE_SIZE; i++) {
        if (!machine_catalog_preview(states, index, &previews[i])) {
          fprintf(stderr, "Could not generate machine preview\n");
          result = 1;
          goto cleanup;
        }
        mpz_add_ui(index, index, 1);
      }
      reload = false;
      dirty = true;
    }
    if (dirty) {
      SDL_SetWindowMinimumSize(window, states >= 3 ? 1152 : 1024, 768);
      char title[256], total[32], page_number[32], page_count[32], selection[32];
      number_label(count, total);
      number_label(pages, page_count);
      mpz_add_ui(index, page, 1);
      number_label(index, page_number);
      mpz_add_ui(index, selected, 1);
      number_label(index, selection);
      snprintf(title, sizeof title, "All %s %lu-state machines | page %s/%s | selected %s | %.60s",
               total, states, page_number, page_count, selection,
               previews[mpz_fdiv_ui(selected, PAGE_SIZE)].instructions);
      SDL_SetWindowTitle(window, title);
      draw_grid(renderer, previews, states, count, pages, page, selected, input, editing, invalid);
      dirty = false;
    }

    SDL_Event event;
    if (!SDL_WaitEvent(&event)) {
      fprintf(stderr, "Could not read grid event: %s\n", SDL_GetError());
      result = 1;
      goto cleanup;
    }
    if (event.type == SDL_QUIT)
      goto cleanup;
    if (event.type == SDL_WINDOWEVENT)
      dirty = true;

    mpz_set(next_page, page);
    mpz_set_si(next_selected, -1);
    unsigned long next_states = states;
    if (editing && event.type == SDL_TEXTINPUT) {
      const char *text = event.text.text;
      for (; *text; text++) {
        if (*text < '0' || *text > '9')
          continue;
        if (replace_input) {
          input[0] = '\0';
          replace_input = false;
        }
        size_t length = strlen(input);
        if (length < 20) {
          input[length] = *text;
          input[length + 1] = '\0';
        }
      }
      invalid = false;
      dirty = true;
    }
    if (event.type == SDL_KEYDOWN) {
      if (editing) {
        switch (event.key.keysym.sym) {
        case SDLK_BACKSPACE:
          if (replace_input)
            input[0] = '\0';
          else if (*input)
            input[strlen(input) - 1] = '\0';
          replace_input = false;
          invalid = false;
          break;
        case SDLK_RETURN: case SDLK_KP_ENTER:
          invalid = !machine_catalog_parse_states(input, &next_states);
          if (!invalid) {
            editing = false;
            SDL_StopTextInput();
          }
          break;
        case SDLK_ESCAPE:
          editing = false;
          invalid = false;
          snprintf(input, sizeof input, "%lu", states);
          SDL_StopTextInput();
          break;
        }
        dirty = true;
      } else {
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE: goto cleanup;
        case SDLK_PAGEUP: case SDLK_LEFT: mpz_sub_ui(next_page, page, 1); break;
        case SDLK_PAGEDOWN: case SDLK_RIGHT: mpz_add_ui(next_page, page, 1); break;
        case SDLK_HOME: mpz_set_ui(next_page, 0); mpz_set_ui(next_selected, 0); break;
        case SDLK_END: mpz_sub_ui(next_page, pages, 1); break;
        }
      }
    }
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
      SDL_Point mouse = {event.button.x, event.button.y};
      int width;
      int height;
      SDL_GetRendererOutputSize(renderer, &width, &height);
      SDL_Rect field = state_control(1, width);
      if (SDL_PointInRect(&mouse, &field)) {
        editing = true;
        replace_input = true;
        invalid = false;
        SDL_StartTextInput();
        SDL_SetTextInputRect(&field);
        dirty = true;
        continue;
      }
      if (editing) {
        editing = false;
        invalid = false;
        snprintf(input, sizeof input, "%lu", states);
        SDL_StopTextInput();
        dirty = true;
      }
      for (int part = 0; part < 3; part += 2) {
        SDL_Rect control = state_control(part, width);
        if (SDL_PointInRect(&mouse, &control)) {
          if (part == 0 && states > 1)
            next_states--;
          else if (part == 2) {
            char candidate[32];
            snprintf(candidate, sizeof candidate, "%lu", states + 1);
            machine_catalog_parse_states(candidate, &next_states);
          }
        }
      }
      for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 5)
          continue;
        SDL_Rect button = navigation_button(i);
        if (!SDL_PointInRect(&mouse, &button))
          continue;
        switch (i) {
        case 0: mpz_set_ui(next_page, 0); mpz_set_ui(next_selected, 0); break;
        case 1: mpz_sub_ui(next_page, page, 1); break;
        case 2: mpz_add_ui(next_page, page, 1); break;
        case 3: mpz_sub_ui(next_page, pages, 1); break;
        case 6: case 7: case 8: case 9:
          if (group_target(states, selected, i, next_selected))
            mpz_fdiv_q_ui(next_page, next_selected, PAGE_SIZE);
          break;
        }
      }
      for (int i = 0; i < PAGE_SIZE; i++) {
        SDL_Rect tile = tile_rect(i, width, height);
        if (SDL_PointInRect(&mouse, &tile)) {
          mpz_mul_ui(selected, page, PAGE_SIZE);
          mpz_add_ui(selected, selected, (unsigned long)i);
          const TuringMachine *machine = turing_init(previews[i].instructions, false);
          TapeViewResult view_result = tape_view_run(renderer, machine, true, previews[i].steps);
          if (view_result != TAPE_VIEW_BACK) {
            result = view_result == TAPE_VIEW_ERROR ? 1 : 0;
            goto cleanup;
          }
          dirty = true;
          break;
        }
      }
    }
    if (next_states != states) {
      states = next_states;
      if (!machine_catalog_count(states, count)) {
        result = 1;
        goto cleanup;
      }
      mpz_divexact_ui(pages, count, PAGE_SIZE);
      mpz_set_ui(page, 0);
      mpz_set_ui(selected, 0);
      snprintf(input, sizeof input, "%lu", states);
      reload = true;
    } else {
      if (mpz_sgn(next_page) < 0)
        mpz_set_ui(next_page, 0);
      if (mpz_cmp(next_page, pages) >= 0)
        mpz_sub_ui(next_page, pages, 1);
      if (mpz_sgn(next_selected) >= 0) {
        mpz_set(selected, next_selected);
        dirty = true;
      } else if (mpz_cmp(next_page, page) != 0) {
        mpz_mul_ui(selected, next_page, PAGE_SIZE);
      }
      if (mpz_cmp(next_page, page) != 0) {
        mpz_set(page, next_page);
        reload = true;
      }
    }
  }
cleanup:
  SDL_StopTextInput();
  for (int i = 0; i < PAGE_SIZE; i++)
    machine_preview_clear(&previews[i]);
  mpz_clears(count, pages, page, selected, next_page, next_selected, index, NULL);
  return result;
}
