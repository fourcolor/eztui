/* tree_demo.c — demonstrates comp/group/scene, pub/sub events, focus, timers */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "eztui.h"

/* =========================================================================
 * Shared styles
 * ========================================================================= */
#define STYLE_NORMAL   EZT_STYLE(EZT_WHITE,       EZT_BLUE,         EZT_ATTR_NONE)
#define STYLE_FOCUSED  EZT_STYLE(EZT_BLACK,       EZT_BRIGHT_CYAN,  EZT_ATTR_BOLD)
#define STYLE_TITLE    EZT_STYLE(EZT_BRIGHT_WHITE, EZT_BLUE,        EZT_ATTR_BOLD)
#define STYLE_STATUS   EZT_STYLE(EZT_BLACK,       EZT_BRIGHT_WHITE, EZT_ATTR_NONE)
#define STYLE_LOG      EZT_STYLE(EZT_BRIGHT_WHITE, EZT_COLOR_DEFAULT, EZT_ATTR_NONE)
#define STYLE_HILIGHT  EZT_STYLE(EZT_BLACK,       EZT_BRIGHT_YELLOW, EZT_ATTR_NONE)

/* =========================================================================
 * Event log
 * ========================================================================= */
#define LOG_LINES 8
static char s_log[LOG_LINES][80];
static int  s_log_count = 0;

static void log_push(const char *msg) {
    if (s_log_count < LOG_LINES) {
        strncpy(s_log[s_log_count++], msg, 79);
    } else {
        memmove(s_log[0], s_log[1], (LOG_LINES - 1) * 80);
        strncpy(s_log[LOG_LINES - 1], msg, 79);
    }
}

/* =========================================================================
 * App state
 * ========================================================================= */
static bool s_running    = true;
static int  s_tick_count = 0;

/* =========================================================================
 * Title bar component
 * ========================================================================= */
static void title_render(ezt_comp_t *comp, ezt_rect_t r) {
    const char *text = comp->data;
    ezt_draw_fill(r.x, r.y, r.w, 1, ' ', STYLE_TITLE);
    int pad = (r.w - (int)strlen(text)) / 2;
    if (pad < 0) pad = 0;
    ezt_draw_text(r.x + pad, r.y, text, STYLE_TITLE);
}

/* =========================================================================
 * Input panel component
 * ========================================================================= */
typedef struct {
    ezt_input_t inp;
    char        buf[64];
    char        label[32];
    ezt_sub_t  *key_sub;
    ezt_comp_t *self;
} InputData;

static void input_render(ezt_comp_t *comp, ezt_rect_t r) {
    InputData *d = comp->data;
    bool focused = ezt_is_focused(comp);
    ezt_style_t s = focused ? STYLE_FOCUSED : STYLE_NORMAL;

    ezt_draw_fill(r.x, r.y, r.w, r.h, ' ', s);
    ezt_draw_box(r.x, r.y, r.w, r.h, EZT_BOX_ROUNDED, s);
    ezt_draw_text(r.x + 2, r.y, d->label, s);
    if (r.w > 4 && r.h > 2)
        ezt_input_draw(&d->inp, r.x + 2, r.y + 1, r.w - 4, s);
    if (focused)
        ezt_draw_text(r.x + r.w - 3, r.y + r.h - 1, "[▶]", STYLE_HILIGHT);
}

static void input_key(const ezt_event_t *ev, void *ud) {
    InputData *d = ud;
    if (!ezt_is_focused(d->self)) return;
    const ezt_key_data_t *kd = ev->data;
    if (ezt_input_feed(&d->inp, kd)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "%s submitted: \"%s\"", d->label, d->buf);
        log_push(msg);
        d->inp.len = d->inp.cursor = 0;
        d->buf[0]  = '\0';
    }
}

static void input_destroy(ezt_comp_t *comp) {
    InputData *d = comp->data;
    ezt_off(d->key_sub);
    free(d);
}

static ezt_comp_t *make_input(int layer, const char *label) {
    InputData *d = calloc(1, sizeof(*d));
    strncpy(d->label, label, sizeof(d->label) - 1);
    ezt_input_init(&d->inp, d->buf, sizeof(d->buf));

    ezt_comp_t *c = ezt_comp_new(layer, input_render, d);
    c->focusable  = true;
    c->destroy    = input_destroy;
    d->self       = c;
    d->key_sub    = ezt_on(EZT_EV_KEY, input_key, d);
    return c;
}

/* =========================================================================
 * List panel component
 * ========================================================================= */
static const char *s_fruits[] = {
    "Apple","Banana","Cherry","Date","Elderberry",
    "Fig","Grape","Honeydew","Kiwi","Lemon",
};
#define FRUIT_COUNT 10

typedef struct {
    ezt_list_t  lst;
    ezt_sub_t  *key_sub;
    ezt_comp_t *self;
} ListData;

static void list_render(ezt_comp_t *comp, ezt_rect_t r) {
    ListData *d = comp->data;
    bool focused = ezt_is_focused(comp);
    ezt_style_t s = focused ? STYLE_FOCUSED : STYLE_NORMAL;

    ezt_draw_fill(r.x, r.y, r.w, r.h, ' ', STYLE_NORMAL);
    ezt_draw_box(r.x, r.y, r.w, r.h, EZT_BOX_SINGLE, s);
    ezt_draw_text(r.x + 1, r.y, focused ? " Fruits [*] " : " Fruits ", s);
    if (r.w > 2 && r.h > 2)
        ezt_list_draw(&d->lst, r.x+1, r.y+1, r.w-2, r.h-2,
                      STYLE_NORMAL, STYLE_HILIGHT);
}

static void list_key(const ezt_event_t *ev, void *ud) {
    ListData *d = ud;
    if (!ezt_is_focused(d->self)) return;
    const ezt_key_data_t *kd = ev->data;
    if (kd->key == EZT_KEY_ENTER) {
        char msg[80];
        snprintf(msg, sizeof(msg), "Selected: %s", s_fruits[d->lst.selected]);
        log_push(msg);
    } else {
        ezt_list_feed(&d->lst, kd);
    }
}

static void list_destroy(ezt_comp_t *comp) {
    ListData *d = comp->data;
    ezt_off(d->key_sub);
    free(d);
}

static ezt_comp_t *make_list(int layer) {
    ListData *d = calloc(1, sizeof(*d));
    ezt_list_init(&d->lst, s_fruits, FRUIT_COUNT);

    ezt_comp_t *c = ezt_comp_new(layer, list_render, d);
    c->focusable  = true;
    c->destroy    = list_destroy;
    d->self       = c;
    d->key_sub    = ezt_on(EZT_EV_KEY, list_key, d);
    return c;
}

/* =========================================================================
 * Log panel component
 * ========================================================================= */
static void log_render(ezt_comp_t *comp, ezt_rect_t r) {
    (void)comp;
    ezt_draw_fill(r.x, r.y, r.w, r.h, ' ', STYLE_LOG);
    ezt_draw_box(r.x, r.y, r.w, r.h, EZT_BOX_ASCII,
                 EZT_STYLE(EZT_CYAN, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));
    ezt_draw_text(r.x + 1, r.y, " Event Log ", STYLE_TITLE);
    int inner_h = r.h - 2;
    int start   = s_log_count > inner_h ? s_log_count - inner_h : 0;
    for (int i = start; i < s_log_count; i++) {
        int row = r.y + 1 + (i - start);
        if (row >= r.y + r.h - 1) break;
        ezt_draw_text(r.x + 1, row, s_log[i], STYLE_LOG);
    }
}

/* =========================================================================
 * Status bar component
 * ========================================================================= */
static void status_render(ezt_comp_t *comp, ezt_rect_t r) {
    (void)comp;
    char buf[256];
    snprintf(buf, sizeof(buf),
             " Tab: next  Shift+Tab: prev  Q: quit  Ticks: %d", s_tick_count);
    ezt_draw_fill(r.x, r.y, r.w, 1, ' ', STYLE_STATUS);
    ezt_draw_text(r.x, r.y, buf, STYLE_STATUS);
}

/* =========================================================================
 * Build the scene
 * ========================================================================= */
static ezt_comp_t *s_input1 = NULL;
static ezt_comp_t *s_input2 = NULL;

static void build_scene(void) {
    int sw, sh;
    ezt_get_size(&sw, &sh);

    /* Title bar — layer 0, fixed 1 row */
    static const char *title_text = " eztui comp_demo — comp/group/scene API ";
    ezt_comp_t *title = ezt_comp_new(0, title_render, (void *)title_text);
    title->rect       = EZT_RECT(0, 0, sw, 1);
    ezt_scene_add_comp(title);

    /* Middle group (hbox): list | input column */
    int mid_h = sh - 1 - LOG_LINES - 1;
    if (mid_h < 4) mid_h = 4;

    ezt_group_t *mid = ezt_group_new(1);
    mid->rect        = EZT_RECT(0, 1, sw, mid_h);
    mid->layout_fn   = ezt_layout_hbox;

    /* List — fixed 24 cols */
    ezt_comp_t *list = make_list(0);
    list->hint       = EZT_RECT(0, 0, 24, 0);
    ezt_group_add_comp(mid, list);

    /* Input column (vbox) — fills remaining width */
    ezt_group_t *vcol = ezt_group_new(1);
    vcol->hint        = EZT_RECT(0, 0, 0, 0);  /* flex width */
    vcol->layout_fn   = ezt_layout_vbox;

    s_input1       = make_input(0, " Name ");
    s_input1->hint = EZT_RECT(0, 0, 0, 3);
    ezt_group_add_comp(vcol, s_input1);

    s_input2       = make_input(1, " Email ");
    s_input2->hint = EZT_RECT(0, 0, 0, 3);
    ezt_group_add_comp(vcol, s_input2);

    /* Spacer — flex, fills remaining height */
    ezt_comp_t *spacer = ezt_comp_new(2, NULL, NULL);
    spacer->hint       = EZT_RECT(0, 0, 0, 0);
    ezt_group_add_comp(vcol, spacer);

    ezt_group_add_group(mid, vcol);
    ezt_scene_add_group(mid);

    /* Event log — layer 2 */
    ezt_comp_t *logp = ezt_comp_new(2, log_render, NULL);
    logp->rect       = EZT_RECT(0, 1 + mid_h, sw, LOG_LINES);
    ezt_scene_add_comp(logp);

    /* Status bar — layer 3 */
    ezt_comp_t *status = ezt_comp_new(3, status_render, NULL);
    status->rect       = EZT_RECT(0, sh - 1, sw, 1);
    ezt_scene_add_comp(status);

    /* Run layout for groups */
    ezt_group_layout(mid);
}

/* =========================================================================
 * Global event handlers
 * ========================================================================= */
static void on_key(const ezt_event_t *ev, void *ud) {
    (void)ud;
    const ezt_key_data_t *kd = ev->data;
    if (kd->key == 'q' || kd->key == 'Q') { s_running = false; return; }
    if (kd->key == EZT_KEY_TAB) {
        if (kd->mods & EZT_MOD_SHIFT) ezt_focus_prev();
        else                          ezt_focus_next();
    }
}

static void on_resize(const ezt_event_t *ev, void *ud) {
    (void)ud;
    const ezt_resize_data_t *rd = ev->data;
    ezt_scene_clear();
    build_scene();
    ezt_focus(s_input1);
    char msg[64];
    snprintf(msg, sizeof(msg), "Resized to %dx%d", rd->w, rd->h);
    log_push(msg);
}

static void on_timer(const ezt_event_t *ev, void *ud) {
    (void)ev; (void)ud;
    s_tick_count++;
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(void) {
    if (ezt_init() < 0) { fprintf(stderr, "ezt_init failed\n"); return 1; }

    build_scene();
    log_push("Started — Tab to switch focus, Q to quit");

    ezt_sub_t *ksub = ezt_on(EZT_EV_KEY,    on_key,    NULL);
    ezt_sub_t *rsub = ezt_on(EZT_EV_RESIZE, on_resize, NULL);

    ezt_timer_t *tmr     = ezt_timer_new(1000, true, NULL);
    ezt_sub_t   *tmr_sub = ezt_on(EZT_EV_TIMER, on_timer, NULL);

    ezt_focus(s_input1);

    while (s_running) {
        int sw, sh;
        ezt_get_size(&sw, &sh);
        ezt_clear();
        ezt_scene_render();
        ezt_refresh();
        ezt_tick(100);
    }

    ezt_off(ksub);
    ezt_off(rsub);
    ezt_off(tmr_sub);
    ezt_timer_free(tmr);
    ezt_scene_clear();

    ezt_cleanup();
    return 0;
}
