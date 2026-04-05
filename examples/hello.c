/* hello.c — minimal eztui example */

#include <stddef.h>
#include <stdbool.h>
#include "eztui.h"

static bool s_running = true;
static int  s_w, s_h;
static int  s_bw = 30, s_bh = 7;

static void draw(void) {
    int bx = (s_w - s_bw) / 2, by = (s_h - s_bh) / 2;
    ezt_clear();
    ezt_draw_box(bx, by, s_bw, s_bh, EZT_BOX_ROUNDED,
                 EZT_STYLE(EZT_BRIGHT_CYAN, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));
    ezt_draw_text(bx + 2, by + 2, "Hello, eztui!",
                  EZT_STYLE(EZT_BRIGHT_WHITE, EZT_COLOR_DEFAULT, EZT_ATTR_BOLD));
    ezt_draw_text(bx + 2, by + 3, "你好，世界！ こんにちは",
                  EZT_STYLE(EZT_BRIGHT_YELLOW, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));
    ezt_draw_text(bx + 2, by + 5, "Press Q to quit",
                  EZT_STYLE(EZT_BRIGHT_BLACK, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));
    ezt_refresh();
}

static void on_key(const ezt_event_t *ev, void *ud) {
    (void)ud;
    const ezt_key_data_t *kd = ev->data;
    if (kd->key == 'q' || kd->key == 'Q' || kd->key == EZT_KEY_ESC)
        s_running = false;
}

static void on_resize(const ezt_event_t *ev, void *ud) {
    (void)ud;
    const ezt_resize_data_t *rd = ev->data;
    s_w = rd->w; s_h = rd->h;
}

int main(void) {
    if (ezt_init() < 0) return 1;
    ezt_get_size(&s_w, &s_h);

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
