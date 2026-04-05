/* event.c — global pub/sub event system */

#include <stdlib.h>
#include <string.h>
#include "event_internal.h"

/* -------------------------------------------------------------------------
 * Subscription list  (doubly-linked for O(1) removal)
 * ------------------------------------------------------------------------- */
struct ezt_sub {
    uint32_t       ev_id;
    ezt_handler_fn fn;
    void          *userdata;
    bool           dead;      /* marked during dispatch, freed after */
    ezt_sub_t     *prev;
    ezt_sub_t     *next;
};

static ezt_sub_t *s_head   = NULL;
static int        s_depth  = 0;   /* dispatch re-entrancy depth */

/* -------------------------------------------------------------------------
 * Custom event registry
 * ------------------------------------------------------------------------- */
#define MAX_CUSTOM_EVENTS 256

static uint32_t s_next_id = EZT_EV_USER;

uint32_t ezt_event_register(const char *name) {
    (void)name; /* name is informational only */
    return s_next_id++;
}

/* -------------------------------------------------------------------------
 * Subscribe
 * ------------------------------------------------------------------------- */
ezt_sub_t *ezt_on(uint32_t ev_id, ezt_handler_fn fn, void *userdata) {
    ezt_sub_t *s = calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->ev_id    = ev_id;
    s->fn       = fn;
    s->userdata = userdata;
    /* Prepend */
    s->next = s_head;
    if (s_head) s_head->prev = s;
    s_head = s;
    return s;
}

/* -------------------------------------------------------------------------
 * Unsubscribe
 * ------------------------------------------------------------------------- */
void ezt_off(ezt_sub_t *sub) {
    if (!sub) return;
    if (s_depth > 0) {
        /* Inside dispatch: defer removal */
        sub->dead = true;
        return;
    }
    if (sub->prev) sub->prev->next = sub->next;
    else           s_head          = sub->next;
    if (sub->next) sub->next->prev = sub->prev;
    free(sub);
}

/* -------------------------------------------------------------------------
 * Emit
 * ------------------------------------------------------------------------- */
void ezt_emit(uint32_t ev_id, void *data) {
    ezt_event_t ev = { .id = ev_id, .data = data };

    s_depth++;
    for (ezt_sub_t *s = s_head; s; s = s->next) {
        if (!s->dead && s->ev_id == ev_id)
            s->fn(&ev, s->userdata);
    }
    s_depth--;

    /* Clean up deferred removals once we're back at the top level */
    if (s_depth == 0) {
        ezt_sub_t *s = s_head;
        while (s) {
            ezt_sub_t *next = s->next;
            if (s->dead) {
                if (s->prev) s->prev->next = s->next;
                else         s_head        = s->next;
                if (s->next) s->next->prev = s->prev;
                free(s);
            }
            s = next;
        }
    }
}

/* -------------------------------------------------------------------------
 * Init (called by ezt_init)
 * ------------------------------------------------------------------------- */
void event_init(void) {
    s_head    = NULL;
    s_depth   = 0;
    s_next_id = EZT_EV_USER;
}
