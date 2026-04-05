/* eztui.c — library init / cleanup / main loop tick */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include "term_internal.h"
#include "screen_internal.h"
#include "event_internal.h"
#include "scene_internal.h"
#include "timer_internal.h"
#include "input_internal.h"
#include "eztui.h"

int ezt_init(void) {
    if (term_platform_init() < 0) return -1;

    event_init();
    timer_init();
    scene_init();

    int w, h;
    term_platform_get_size(&w, &h);
    screen_init(w, h);

    term_screen_alt_enter();
    term_cursor_hide();
    term_mouse_enable();
    term_buf_flush();

    return 0;
}

void ezt_cleanup(void) {
    term_mouse_disable();
    term_cursor_show();
    term_screen_alt_leave();
    term_buf_flush();
    term_platform_cleanup();
}

/* -------------------------------------------------------------------------
 * Main loop tick: check timers → poll input → emit events
 * ------------------------------------------------------------------------- */
bool ezt_tick(int timeout_ms) {
    /* Fire any expired timers first */
    timer_tick();

    /* Compute effective timeout: min(requested, time-to-next-timer) */
    int effective = timeout_ms;
    int tnext     = timer_next_ms();
    if (tnext >= 0 && (effective < 0 || tnext < effective))
        effective = tnext;

    /* Read and emit one input event */
    bool got = input_read_and_emit(effective);

    /* Drain any additional buffered input without blocking */
    while (input_read_and_emit(0)) {}

    /* Fire timers that may have expired during the wait */
    timer_tick();

    return got;
}
