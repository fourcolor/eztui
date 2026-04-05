/* scene.c — global top-level container */

#include <stdlib.h>
#include <string.h>
#include "eztui.h"
#include "group_internal.h"

#define SCENE_INIT_CAP 16

static EztItem *s_items = NULL;
static int      s_count = 0;
static int      s_cap   = 0;
static int      s_ins   = 0;

static int item_layer(const EztItem *it) {
    return it->is_group ? it->group->layer : it->comp->layer;
}

static int item_cmp(const void *a, const void *b) {
    const EztItem *ia = a, *ib = b;
    int la = item_layer(ia), lb = item_layer(ib);
    if (la != lb) return la - lb;
    return ia->insert_idx - ib->insert_idx;
}

static EztItem *scene_push(void) {
    if (s_count >= s_cap) {
        int newcap = s_cap ? s_cap * 2 : SCENE_INIT_CAP;
        EztItem *nb = realloc(s_items, (size_t)newcap * sizeof(EztItem));
        if (!nb) return NULL;
        s_items = nb;
        s_cap   = newcap;
    }
    return &s_items[s_count++];
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */
void ezt_scene_add_comp(ezt_comp_t *comp) {
    if (!comp) return;
    EztItem *slot = scene_push();
    if (!slot) return;
    slot->is_group   = false;
    slot->insert_idx = s_ins++;
    slot->comp       = comp;
}

void ezt_scene_add_group(ezt_group_t *group) {
    if (!group) return;
    EztItem *slot = scene_push();
    if (!slot) return;
    slot->is_group   = true;
    slot->insert_idx = s_ins++;
    slot->group      = group;
}

void ezt_scene_remove_comp(ezt_comp_t *comp) {
    for (int i = 0; i < s_count; i++) {
        if (!s_items[i].is_group && s_items[i].comp == comp) {
            memmove(&s_items[i], &s_items[i + 1],
                    (size_t)(s_count - i - 1) * sizeof(EztItem));
            s_count--;
            return;
        }
    }
}

void ezt_scene_remove_group(ezt_group_t *group) {
    for (int i = 0; i < s_count; i++) {
        if (s_items[i].is_group && s_items[i].group == group) {
            memmove(&s_items[i], &s_items[i + 1],
                    (size_t)(s_count - i - 1) * sizeof(EztItem));
            s_count--;
            return;
        }
    }
}

void ezt_scene_clear(void) {
    s_count = 0;
    s_ins   = 0;
}

void ezt_scene_render(void) {
    if (s_count == 0) return;

    EztItem *sorted = malloc((size_t)s_count * sizeof(EztItem));
    if (!sorted) return;
    memcpy(sorted, s_items, (size_t)s_count * sizeof(EztItem));
    qsort(sorted, (size_t)s_count, sizeof(EztItem), item_cmp);

    for (int i = 0; i < s_count; i++) {
        if (sorted[i].is_group) {
            group_render(sorted[i].group);
        } else {
            ezt_comp_t *c = sorted[i].comp;
            if (c->render) c->render(c, c->rect);
        }
    }
    free(sorted);
}

/* -------------------------------------------------------------------------
 * Focusable collection (used by focus.c)
 * ------------------------------------------------------------------------- */
#define MAX_FOCUSABLE 256

void scene_collect_focusable(ezt_comp_t **out, int *count) {
    *count = 0;
    if (s_count == 0) return;

    EztItem *sorted = malloc((size_t)s_count * sizeof(EztItem));
    if (!sorted) return;
    memcpy(sorted, s_items, (size_t)s_count * sizeof(EztItem));
    qsort(sorted, (size_t)s_count, sizeof(EztItem), item_cmp);

    for (int i = 0; i < s_count && *count < MAX_FOCUSABLE; i++) {
        if (sorted[i].is_group) {
            group_collect_focusable(sorted[i].group, out, count, MAX_FOCUSABLE);
        } else {
            ezt_comp_t *c = sorted[i].comp;
            if (c->focusable) out[(*count)++] = c;
        }
    }
    free(sorted);
}

/* -------------------------------------------------------------------------
 * Internal init (called from ezt_init)
 * ------------------------------------------------------------------------- */
void scene_init(void) {
    free(s_items);
    s_items = NULL;
    s_count = s_cap = s_ins = 0;
}
