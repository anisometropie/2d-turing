#ifndef TURING_H
#define TURING_H

#include "state_name.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  const char *instructions;
  unsigned char *buffer;
  unsigned char *tape;
  TuringState state;
  int64_t head;
  size_t capacity;
  unsigned char blank_symbol;
} TuringMachine;


const TuringMachine *turing_init(const char *instructions, bool fill_with_ones);
const TuringMachine *turing_init_with_state(const char *instructions, bool fill_with_ones, TuringState starting_state);
typedef enum {
  STEP_RUNNING,
  STEP_HALTED,
  STEP_MISSING_TRANSITION
} StepResult;

StepResult step(void);
bool turing_can_step(void);

#endif
