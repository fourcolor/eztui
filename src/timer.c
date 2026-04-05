/* timer.c — repeating and one-shot timers */

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <time.h>
#include "timer_internal.h"
#include "eztui.h"

/* -------------------------------------------------------------------------
 * Monotonic clock helper (ms)
 * ------------------------------------------------------------------------- */
static int64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* -------------------------------------------------------------------------
 * Timer list
 * ------------------------------------------------------------------------- */
struct ezt_timer {
    int            interval_ms;
    bool           repeat;
    void          *userdata;
    int64_t        next_fire;   /* absolute monotonic ms */
    bool           dead;
    ezt_timer_t   *next;
};

static ezt_timer_t *s_timers = NULL;

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */
ezt_timer_t *ezt_timer_new(int interval_ms, bool repeat, void *userdata) {
    ezt_timer_t *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    t->interval_ms = interval_ms;
    t->repeat      = repeat;
    t->userdata    = userdata;
    t->next_fire   = now_ms() + interval_ms;
    t->next        = s_timers;
    s_timers       = t;
    return t;
}

void ezt_timer_free(ezt_timer_t *t) {
    if (!t) return;
    /* Mark dead; removed on next timer_tick pass */
    t->dead = true;
}

/* -------------------------------------------------------------------------
 * Internal
 * ------------------------------------------------------------------------- */
void timer_init(void) {
    s_timers = NULL;
}

int timer_next_ms(void) {
    int64_t now  = now_ms();
    int64_t best = -1;
    for (ezt_timer_t *t = s_timers; t; t = t->next) {
        if (t->dead) continue;
        int64_t rem = t->next_fire - now;
        if (rem < 0) rem = 0;
        if (best < 0 || rem < best) best = rem;
    }
    return (int)(best);
}

void timer_tick(void) {
    int64_t now = now_ms();

    for (ezt_timer_t *t = s_timers; t; t = t->next) {
        if (t->dead) continue;
        if (now >= t->next_fire) {
            /* Emit EZT_EV_TIMER with userdata */
            ezt_timer_data_t td = { .userdata = t->userdata };
            ezt_emit(EZT_EV_TIMER, &td);

            if (t->repeat)
                t->next_fire = now + t->interval_ms;
            else
                t->dead = true;
        }
    }

    /* Sweep dead timers */
    ezt_timer_t **pp = &s_timers;
    while (*pp) {
        if ((*pp)->dead) {
            ezt_timer_t *dead = *pp;
            *pp = dead->next;
            free(dead);
        } else {
            pp = &(*pp)->next;
        }
    }
}
