/* group.c — group: a container with layer-ordered children and optional layout */

#include <stdlib.h>
#include <string.h>
#include "eztui.h"
#include "group_internal.h"

/* -------------------------------------------------------------------------
 * Item helpers
 * ------------------------------------------------------------------------- */
static int item_layer(const EztItem *it) {
    return it->is_group ? it->group->layer : it->comp->layer;
}

/* Grow items array by one slot, return pointer to new slot or NULL. */
static EztItem *items_push(ezt_group_t *g) {
    if (g->_count >= g->_cap) {
        int newcap = g->_cap ? g->_cap * 2 : 4;
        EztItem *nb = realloc(g->_items, (size_t)newcap * sizeof(EztItem));
        if (!nb) return NULL;
        g->_items = nb;
        g->_cap   = newcap;
    }
    return &((EztItem *)g->_items)[g->_count++];
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */
ezt_group_t *ezt_group_new(int layer) {
    ezt_group_t *g = calloc(1, sizeof(*g));
    if (!g) return NULL;
    g->layer = layer;
    return g;
}

void ezt_group_free(ezt_group_t *group) {
    if (!group) return;
    if (group->destroy) group->destroy(group);
    free(group->_items);
    free(group);
}

void ezt_group_add_comp(ezt_group_t *group, ezt_comp_t *comp) {
    if (!group || !comp) return;
    EztItem *slot = items_push(group);
    if (!slot) return;
    slot->is_group   = false;
    slot->insert_idx = group->_ins++;
    slot->comp       = comp;
}

void ezt_group_add_group(ezt_group_t *group, ezt_group_t *child) {
    if (!group || !child) return;
    EztItem *slot = items_push(group);
    if (!slot) return;
    slot->is_group   = true;
    slot->insert_idx = group->_ins++;
    slot->group      = child;
}

/* -------------------------------------------------------------------------
 * Layout
 * ------------------------------------------------------------------------- */
void ezt_group_layout(ezt_group_t *group) {
    if (!group) return;
    if (group->layout_fn)
        group->layout_fn(group);
    /* Recurse into child groups */
    EztItem *items = group->_items;
    for (int i = 0; i < group->_count; i++) {
        if (items[i].is_group)
            ezt_group_layout(items[i].group);
    }
}

/* -------------------------------------------------------------------------
 * Render helpers (used by scene.c)
 * ------------------------------------------------------------------------- */

/* Comparison for stable sort: primary = layer, secondary = insert_idx */
static int item_cmp(const void *a, const void *b) {
    const EztItem *ia = a, *ib = b;
    int la = item_layer(ia), lb = item_layer(ib);
    if (la != lb) return la - lb;
    return ia->insert_idx - ib->insert_idx;
}

void group_render(const ezt_group_t *group) {
    if (!group) return;
    int n = group->_count;
    if (n == 0) return;

    /* Sort a temporary copy so we don't mutate the insertion-order array */
    EztItem *sorted = malloc((size_t)n * sizeof(EztItem));
    if (!sorted) return;
    memcpy(sorted, group->_items, (size_t)n * sizeof(EztItem));
    qsort(sorted, (size_t)n, sizeof(EztItem), item_cmp);

    for (int i = 0; i < n; i++) {
        if (sorted[i].is_group) {
            group_render(sorted[i].group);
        } else {
            ezt_comp_t *c = sorted[i].comp;
            if (c->render) c->render(c, c->rect);
        }
    }
    free(sorted);
}

/* Collect all focusable comps in render order (layer order, DFS) */
void group_collect_focusable(const ezt_group_t *group,
                              ezt_comp_t **out, int *count, int cap) {
    if (!group) return;
    int n = group->_count;
    if (n == 0) return;

    EztItem *sorted = malloc((size_t)n * sizeof(EztItem));
    if (!sorted) return;
    memcpy(sorted, group->_items, (size_t)n * sizeof(EztItem));
    qsort(sorted, (size_t)n, sizeof(EztItem), item_cmp);

    for (int i = 0; i < n && *count < cap; i++) {
        if (sorted[i].is_group) {
            group_collect_focusable(sorted[i].group, out, count, cap);
        } else {
            ezt_comp_t *c = sorted[i].comp;
            if (c->focusable) out[(*count)++] = c;
        }
    }
    free(sorted);
}
