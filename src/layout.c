/* layout.c — built-in vbox / hbox layout functions for groups */

#include "eztui.h"
#include "group_internal.h"

/* -------------------------------------------------------------------------
 * vbox: stack children vertically in insertion order.
 * hint.h == 0 → flex (share remaining height equally).
 * hint.w == 0 → full group width; hint.w > 0 → exact width (left-aligned).
 * ------------------------------------------------------------------------- */
void ezt_layout_vbox(ezt_group_t *group) {
    ezt_rect_t r    = group->rect;
    EztItem   *items = group->_items;
    int        n    = group->_count;

    int fixed_h    = 0;
    int flex_count = 0;
    for (int i = 0; i < n; i++) {
        int h = items[i].is_group ? items[i].group->hint.h
                                  : items[i].comp->hint.h;
        if (h > 0) fixed_h += h;
        else       flex_count++;
    }

    int remaining = r.h - fixed_h;
    if (remaining < 0) remaining = 0;
    int flex_h = flex_count > 0 ? remaining / flex_count : 0;

    int y = r.y;
    for (int i = 0; i < n; i++) {
        int hint_h = items[i].is_group ? items[i].group->hint.h
                                       : items[i].comp->hint.h;
        int hint_w = items[i].is_group ? items[i].group->hint.w
                                       : items[i].comp->hint.w;
        int ch = hint_h > 0 ? hint_h : flex_h;
        int cw = hint_w > 0 ? hint_w : r.w;
        ezt_rect_t cr = EZT_RECT(r.x, y, cw, ch);

        if (items[i].is_group) items[i].group->rect = cr;
        else                   items[i].comp->rect  = cr;

        y += ch;
    }
}

/* -------------------------------------------------------------------------
 * hbox: stack children horizontally in insertion order.
 * hint.w == 0 → flex; hint.h == 0 → full group height.
 * ------------------------------------------------------------------------- */
void ezt_layout_hbox(ezt_group_t *group) {
    ezt_rect_t r     = group->rect;
    EztItem   *items  = group->_items;
    int        n     = group->_count;

    int fixed_w    = 0;
    int flex_count = 0;
    for (int i = 0; i < n; i++) {
        int w = items[i].is_group ? items[i].group->hint.w
                                  : items[i].comp->hint.w;
        if (w > 0) fixed_w += w;
        else       flex_count++;
    }

    int remaining = r.w - fixed_w;
    if (remaining < 0) remaining = 0;
    int flex_w = flex_count > 0 ? remaining / flex_count : 0;

    int x = r.x;
    for (int i = 0; i < n; i++) {
        int hint_w = items[i].is_group ? items[i].group->hint.w
                                       : items[i].comp->hint.w;
        int hint_h = items[i].is_group ? items[i].group->hint.h
                                       : items[i].comp->hint.h;
        int cw = hint_w > 0 ? hint_w : flex_w;
        int ch = hint_h > 0 ? hint_h : r.h;
        ezt_rect_t cr = EZT_RECT(x, r.y, cw, ch);

        if (items[i].is_group) items[i].group->rect = cr;
        else                   items[i].comp->rect  = cr;

        x += cw;
    }
}
