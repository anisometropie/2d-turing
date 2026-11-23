#ifndef STATE_NAME_H
#define STATE_NAME_H

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

typedef size_t TuringState;
enum { STATE_NAME_CAPACITY = sizeof(TuringState) * CHAR_BIT + 1 };

// H is reserved for halt. Existing single-letter state values stay compatible.
TuringState state_from_index(size_t index);
size_t state_index(TuringState state);
bool state_parse(const char *name, size_t length, TuringState *state);
void state_format(TuringState state, char name[STATE_NAME_CAPACITY]);

#endif
