/* screen.c — double-buffered cell screen with diff rendering */

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "screen_internal.h"
#include "term_internal.h"
#include "unicode_internal.h"
#include "color_internal.h"

/* -------------------------------------------------------------------------
 * Cell
 * A cell holds one Unicode codepoint (which may be 0 for the right half
 * of a wide character), plus its visual attributes.
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t cp;    /* Unicode codepoint; 0 = wide-char placeholder */
    uint32_t fg;
    uint32_t bg;
    int      attr;
} Cell;

#define MAX_W 512
#define MAX_H 256

static Cell  s_front[MAX_H][MAX_W];
static Cell  s_back [MAX_H][MAX_W];

static int s_w, s_h;
static int s_cur_x, s_cur_y;
static uint32_t s_cur_fg, s_cur_bg;
static int      s_cur_attr;


/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */
static Cell *back_cell(int x, int y) {
    if (x < 0 || x >= s_w || y < 0 || y >= s_h) return NULL;
    return &s_back[y][x];
}

static void put_cell(int x, int y, uint32_t cp) {
    Cell *c = back_cell(x, y);
    if (!c) return;
    c->cp   = cp;
    c->fg   = s_cur_fg;
    c->bg   = s_cur_bg;
    c->attr = s_cur_attr;
}

/* -------------------------------------------------------------------------
 * Init / cleanup (called by eztui.c)
 * ------------------------------------------------------------------------- */
void screen_init(int w, int h) {
    s_w = w < MAX_W ? w : MAX_W;
    s_h = h < MAX_H ? h : MAX_H;
    s_cur_fg   = EZT_COLOR_DEFAULT;
    s_cur_bg   = EZT_COLOR_DEFAULT;
    s_cur_attr = EZT_ATTR_NONE;

    Cell blank = { .cp = ' ', .fg = EZT_COLOR_DEFAULT,
                   .bg = EZT_COLOR_DEFAULT, .attr = EZT_ATTR_NONE };
    for (int y = 0; y < s_h; y++)
        for (int x = 0; x < s_w; x++) {
            s_front[y][x] = blank;
            s_back [y][x] = blank;
        }
}

void screen_resize(int w, int h) {
    screen_init(w, h);
}

/* -------------------------------------------------------------------------
 * Public drawing API
 * ------------------------------------------------------------------------- */
void  ezt_get_size   (int *w, int *h) { *w = s_w; *h = s_h; }
float ezt_cell_aspect(void)           { return term_platform_cell_aspect(); }
void ezt_resize  (int w, int h)  { screen_resize(w, h); }

void ezt_move(int x, int y)       { s_cur_x = x; s_cur_y = y; }
void ezt_color(uint32_t fg, uint32_t bg) { s_cur_fg = fg; s_cur_bg = bg; }
void ezt_attr(int a)              { s_cur_attr = a; }

void ezt_putc(uint32_t cp) {
    int w = ucp_width(cp);
    if (w == 0) return; /* combining: skip for now */

    put_cell(s_cur_x, s_cur_y, cp);
    if (w == 2) {
        /* Right placeholder — codepoint 0, same colors */
        Cell *r = back_cell(s_cur_x + 1, s_cur_y);
        if (r) {
            r->cp   = 0;
            r->fg   = s_cur_fg;
            r->bg   = s_cur_bg;
            r->attr = s_cur_attr;
        }
    }
    s_cur_x += w;
}

void ezt_puts(const char *utf8) {
    uint32_t cp;
    while ((cp = utf8_decode(&utf8)) != 0)
        ezt_putc(cp);
}

void ezt_fill(int x, int y, int w, int h, uint32_t cp) {
    uint32_t saved_fg   = s_cur_fg;
    uint32_t saved_bg   = s_cur_bg;
    int      saved_attr = s_cur_attr;

    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            s_cur_x = col; s_cur_y = row;
            put_cell(col, row, cp);
        }
    }

    s_cur_fg   = saved_fg;
    s_cur_bg   = saved_bg;
    s_cur_attr = saved_attr;
}

void ezt_clear(void) {
    Cell blank = { .cp = ' ', .fg = EZT_COLOR_DEFAULT,
                   .bg = EZT_COLOR_DEFAULT, .attr = EZT_ATTR_NONE };
    for (int y = 0; y < s_h; y++)
        for (int x = 0; x < s_w; x++)
            s_back[y][x] = blank;
}

/* -------------------------------------------------------------------------
 * Diff render
 * ------------------------------------------------------------------------- */
static uint32_t s_last_fg, s_last_bg;
static int      s_last_attr;
static int      s_last_x, s_last_y;
static bool     s_first_render = true;

static void emit_cell(int x, int y, const Cell *c) {
    ezt_color_cap_t cap = color_detect();
    uint32_t fg = color_downgrade(c->fg, cap);
    uint32_t bg = color_downgrade(c->bg, cap);

    /* Move if not immediately after previous cell */
    if (x != s_last_x || y != s_last_y)
        term_cursor_move(x, y);

    /* Reset + re-emit attrs (cheap: only one CSI per changed cell) */
    term_sgr_reset();
    if (c->attr) term_sgr_attr(c->attr);
    term_sgr_fg(fg);
    term_sgr_bg(bg);

    if (c->cp == 0) {
        /* Wide-char placeholder — don't print, terminal already advanced */
        s_last_x = x + 1;
        s_last_y = y;
        return;
    }

    char buf[4];
    int n = utf8_encode(c->cp, buf);
    term_buf_write(buf, n);

    int cw = ucp_width(c->cp);
    s_last_x = x + cw;
    s_last_y = y;

    s_last_fg   = fg;
    s_last_bg   = bg;
    s_last_attr = c->attr;
    (void)s_last_fg; (void)s_last_bg; (void)s_last_attr;
}

void ezt_refresh(void) {
    if (s_first_render) {
        s_last_x = -1; s_last_y = -1;
        s_first_render = false;
    }

    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            Cell *b = &s_back [y][x];
            Cell *f = &s_front[y][x];
            if (b->cp   == f->cp   &&
                b->fg   == f->fg   &&
                b->bg   == f->bg   &&
                b->attr == f->attr) continue;
            emit_cell(x, y, b);
            *f = *b;
        }
    }

    term_sgr_reset();
    term_buf_flush();
}


/* -------------------------------------------------------------------------
 * Style-based high-level draw API (stateless)
 * ------------------------------------------------------------------------- */
void ezt_draw_text(int x, int y, const char *utf8, ezt_style_t style) {
    ezt_move(x, y);
    ezt_color(style.fg, style.bg);
    ezt_attr(style.attr);
    ezt_puts(utf8);
}

void ezt_draw_fill(int x, int y, int w, int h,
                   uint32_t cp, ezt_style_t style) {
    uint32_t saved_fg = s_cur_fg, saved_bg = s_cur_bg;
    int saved_attr = s_cur_attr;
    ezt_color(style.fg, style.bg);
    ezt_attr(style.attr);
    ezt_fill(x, y, w, h, cp);
    ezt_color(saved_fg, saved_bg);
    ezt_attr(saved_attr);
}
