#pragma once
#include "eztui.h"

/* Internal item slot used inside group and scene */
typedef struct {
    bool  is_group;
    int   insert_idx;
    union {
        ezt_comp_t  *comp;
        ezt_group_t *group;
    };
} EztItem;

/* Render a group's children in layer order (recursive) */
void group_render(const ezt_group_t *group);

/* Collect all focusable comps from a group in render order */
void group_collect_focusable(const ezt_group_t *group,
                              ezt_comp_t **out, int *count, int cap);
