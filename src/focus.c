/* focus.c — software focus management */

#include <stddef.h>
#include "eztui.h"
#include "scene_internal.h"

static ezt_comp_t *s_focused = NULL;

void ezt_focus(ezt_comp_t *comp) {
    if (s_focused == comp) return;
    if (s_focused) ezt_emit(EZT_EV_BLUR,  s_focused);
    s_focused = comp;
    if (s_focused) ezt_emit(EZT_EV_FOCUS, s_focused);
}

ezt_comp_t *ezt_focused   (void)             { return s_focused; }
bool        ezt_is_focused(ezt_comp_t *comp) { return comp && comp == s_focused; }

/* -------------------------------------------------------------------------
 * Cycle focus through all focusable comps in scene render order.
 * ------------------------------------------------------------------------- */
#define MAX_FOCUSABLE 256

static void cycle(int delta) {
    ezt_comp_t *list[MAX_FOCUSABLE];
    int count = 0;
    scene_collect_focusable(list, &count);
    if (count == 0) return;

    int idx = -1;
    for (int i = 0; i < count; i++) {
        if (list[i] == s_focused) { idx = i; break; }
    }

    int next = (idx + delta % count + count) % count;
    ezt_focus(list[next]);
}

void ezt_focus_next(void) { cycle(+1); }
void ezt_focus_prev(void) { cycle(-1); }
