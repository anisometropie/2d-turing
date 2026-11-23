#include "state_name.h"

#include <stdint.h>

TuringState state_from_index(size_t index) {
  return index + 'A' + (index >= (size_t)('H' - 'A'));
}

size_t state_index(TuringState state) {
  return state - 'A' - (state > 'H');
}

bool state_parse(const char *name, size_t length, TuringState *state) {
  if (length == 0)
    return false;
  size_t value = 0;
  for (size_t i = 0; i < length; i++) {
    if (name[i] < 'A' || name[i] > 'Z')
      return false;
    size_t digit = (size_t)(name[i] - 'A' + 1);
    if (value > (SIZE_MAX - ('A' - 1) - digit) / 26)
      return false;
    value = value * 26 + digit;
  }
  *state = value + 'A' - 1;
  return true;
}

void state_format(TuringState state, char name[STATE_NAME_CAPACITY]) {
  size_t value = state - 'A' + 1;
  size_t length = 0;
  do {
    value--;
    name[length++] = (char)('A' + value % 26);
    value /= 26;
  } while (value);
  name[length] = '\0';
  for (size_t i = 0; i < length / 2; i++) {
    char letter = name[i];
    name[i] = name[length - i - 1];
    name[length - i - 1] = letter;
  }
}
