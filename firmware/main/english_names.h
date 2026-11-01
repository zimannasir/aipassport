#pragma once
#include <stdint.h>

// Returns an English name (not NUL-terminated by contract; use *length) or NULL
// when the id has no English entry. Callers keep the Chinese name as fallback.
const char *english_species_name(uint16_t species, uint8_t *length);
const char *english_move_name(uint16_t move, uint8_t *length);
