/* demo.c — demonstrates list, input box, and basic event handling */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "eztui.h"

static const char *s_items[] = {
    "Apple", "Banana", "Cherry",
    "Durian", "Elderberry", "Fig", "Grape",
    "Honeydew", "Jackfruit", "Kiwi",
};
#define ITEM_COUNT 10

/* -------------------------------------------------------------------------
 * App state
 * ------------------------------------------------------------------------- */
static bool       s_running     = true;
static bool       s_focus_input = false;  /* false = list focused */
static char       s_ibuf[128];
static ezt_input_t s_inp;
static ezt_list_t  s_lst;

/* -------------------------------------------------------------------------
 * Draw
 * ------------------------------------------------------------------------- */
static void draw(void) {
    int w, h;
    ezt_get_size(&w, &h);
    ezt_clear();

    /* Outer box */
    ezt_draw_box(0, 0, w, h, EZT_BOX_SINGLE,
                 EZT_STYLE(EZT_BRIGHT_BLACK, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));

    /* Title */
    ezt_draw_text(2, 0, " eztui demo ",
                  EZT_STYLE(EZT_BRIGHT_CYAN, EZT_COLOR_DEFAULT, EZT_ATTR_BOLD));

    /* Left panel: list */
    int list_w = w / 2 - 2;
    int list_h = h - 4;
    ezt_box_style_t list_box = s_focus_input ? EZT_BOX_SINGLE : EZT_BOX_BOLD;
    uint32_t list_fg = s_focus_input ? EZT_BRIGHT_BLACK : EZT_BRIGHT_CYAN;
    ezt_draw_box(1, 1, list_w + 2, list_h + 2, list_box,
                 EZT_STYLE(list_fg, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));
    ezt_draw_text(3, 1, " List ",
                  EZT_STYLE(EZT_BRIGHT_WHITE, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));
    ezt_list_draw(&s_lst, 2, 2, list_w, list_h,
                  EZT_STYLE(EZT_WHITE,  EZT_COLOR_DEFAULT, EZT_ATTR_NONE),
                  EZT_STYLE(EZT_BLACK,  EZT_BRIGHT_CYAN,  EZT_ATTR_BOLD));

    /* Right panel: input */
    int input_x = w / 2 + 1;
    int input_w = w - input_x - 2;
    ezt_box_style_t inp_box = s_focus_input ? EZT_BOX_BOLD : EZT_BOX_SINGLE;
    uint32_t inp_fg = s_focus_input ? EZT_BRIGHT_CYAN : EZT_BRIGHT_BLACK;
    ezt_draw_box(input_x - 1, 1, input_w + 2, 3, inp_box,
                 EZT_STYLE(inp_fg, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));
    ezt_draw_text(input_x + 1, 1, " Input ",
                  EZT_STYLE(EZT_BRIGHT_WHITE, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));
    ezt_input_draw(&s_inp, input_x, 2, input_w,
                   EZT_STYLE(EZT_WHITE, EZT_COLOR_16(8), EZT_ATTR_NONE));

    /* Typed text preview */
    if (s_ibuf[0]) {
        char preview[256];
        snprintf(preview, sizeof(preview), " typed: %s", s_ibuf);
        ezt_draw_text(input_x, 5, preview,
                      EZT_STYLE(EZT_BRIGHT_GREEN, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));
    }

    /* Status bar */
    char status[256];
    snprintf(status, sizeof(status),
             " Tab: switch focus  Q: quit  selected: %s",
             s_items[s_lst.selected]);
    ezt_draw_text(1, h - 2, status,
                  EZT_STYLE(EZT_BRIGHT_BLACK, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));

    ezt_refresh();
}

/* -------------------------------------------------------------------------
 * Event handlers
 * ------------------------------------------------------------------------- */
static void on_key(const ezt_event_t *ev, void *ud) {
    (void)ud;
    const ezt_key_data_t *kd = ev->data;

    if (kd->key == 'q' || kd->key == 'Q' || kd->key == EZT_KEY_ESC) {
        s_running = false;
        return;
    }
    if (kd->key == EZT_KEY_TAB) {
        s_focus_input = !s_focus_input;
        return;
    }
    if (s_focus_input)
        ezt_input_feed(&s_inp, kd);
    else
        ezt_list_feed(&s_lst, kd);
}

static void on_resize(const ezt_event_t *ev, void *ud) {
    (void)ev; (void)ud;
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(void) {
    if (ezt_init() < 0) return 1;

    ezt_input_init(&s_inp, s_ibuf, sizeof(s_ibuf));
    ezt_list_init(&s_lst, s_items, ITEM_COUNT);

    ezt_sub_t *ksub = ezt_on(EZT_EV_KEY,    on_key,    NULL);
    ezt_sub_t *rsub = ezt_on(EZT_EV_RESIZE, on_resize, NULL);

    while (s_running) {
        draw();
        ezt_tick(-1);
    }

    ezt_off(ksub);
    ezt_off(rsub);
    ezt_cleanup();
    return 0;
}
