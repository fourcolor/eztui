#pragma once
#include <stdint.h>

void timer_init(void);
/* Returns ms until next timer fires, or -1 if no timers. */
int  timer_next_ms(void);
/* Fire all expired timers. */
void timer_tick(void);
