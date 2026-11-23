#ifndef TURING_H
#define TURING_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  const char *instructions;
  unsigned char *buffer;
  unsigned char *tape;
  char state;
  int64_t head;
  size_t capacity;
} TuringMachine;


const TuringMachine *turing_init(const char *instructions, bool fill_with_ones);
const TuringMachine *turing_init_with_state(const char *instructions, bool fill_with_ones, char starting_state);
typedef enum {
  STEP_RUNNING,
  STEP_HALTED
} StepResult;

StepResult step(void);

#endif
