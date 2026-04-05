#pragma once
#include <stdbool.h>

/* Called by ezt_tick() */
bool input_read_and_emit(int timeout_ms);
