/* widget.c — draw primitives and stateful widget helpers */

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "unicode_internal.h"
#include "screen_internal.h"
#include "eztui.h"

/* =========================================================================
 * Box drawing character sets
 * ========================================================================= */
typedef struct { const char *h, *v, *tl, *tr, *bl, *br; } BoxChars;

static const BoxChars s_box_chars[] = {
    [EZT_BOX_ASCII]   = {"-",  "|",  "+",  "+",  "+",  "+"},
    [EZT_BOX_SINGLE]  = {"─",  "│",  "┌",  "┐",  "└",  "┘"},
    [EZT_BOX_DOUBLE]  = {"═",  "║",  "╔",  "╗",  "╚",  "╝"},
    [EZT_BOX_ROUNDED] = {"─",  "│",  "╭",  "╮",  "╰",  "╯"},
    [EZT_BOX_BOLD]    = {"━",  "┃",  "┏",  "┓",  "┗",  "┛"},
};

/* =========================================================================
 * ezt_draw_box
 * ========================================================================= */
void ezt_draw_box(int x, int y, int w, int h,
                  ezt_box_style_t box_style, ezt_style_t style) {
    if (w < 2 || h < 2) return;
    const BoxChars *bc = &s_box_chars[box_style];
    ezt_color(style.fg, style.bg);
    ezt_attr(style.attr);

    ezt_move(x, y);
    ezt_puts(bc->tl);
    for (int i = 1; i < w - 1; i++) ezt_puts(bc->h);
    ezt_puts(bc->tr);

    for (int row = 1; row < h - 1; row++) {
        ezt_move(x,         y + row); ezt_puts(bc->v);
        ezt_move(x + w - 1, y + row); ezt_puts(bc->v);
    }

    ezt_move(x, y + h - 1);
    ezt_puts(bc->bl);
    for (int i = 1; i < w - 1; i++) ezt_puts(bc->h);
    ezt_puts(bc->br);
}

/* =========================================================================
 * ezt_draw_hbar / ezt_draw_vbar
 * ========================================================================= */
void ezt_draw_hbar(int x, int y, int w, ezt_style_t style) {
    ezt_move(x, y);
    ezt_color(style.fg, style.bg);
    ezt_attr(style.attr);
    for (int i = 0; i < w; i++) ezt_puts("─");
}

void ezt_draw_vbar(int x, int y, int h, ezt_style_t style) {
    ezt_color(style.fg, style.bg);
    ezt_attr(style.attr);
    for (int i = 0; i < h; i++) { ezt_move(x, y + i); ezt_puts("│"); }
}

/* =========================================================================
 * Text input
 * ========================================================================= */
void ezt_input_init(ezt_input_t *inp, char *buf, int buf_size) {
    inp->buf      = buf;
    inp->buf_size = buf_size;
    inp->cursor   = 0;
    inp->len      = 0;
    if (buf_size > 0) buf[0] = '\0';
}

bool ezt_input_feed(ezt_input_t *inp, const ezt_key_data_t *kd) {
    if (kd->key == EZT_KEY_ENTER) return true;

    if (kd->key == EZT_KEY_BACKSPACE) {
        if (inp->cursor == 0) return false;
        int i = inp->cursor - 1;
        while (i > 0 && (inp->buf[i] & 0xC0) == 0x80) i--;
        int del = inp->cursor - i;
        memmove(inp->buf + i, inp->buf + inp->cursor,
                (size_t)(inp->len - inp->cursor));
        inp->len -= del; inp->cursor -= del;
        inp->buf[inp->len] = '\0';
        return false;
    }
    if (kd->key == EZT_KEY_LEFT) {
        if (inp->cursor > 0) {
            inp->cursor--;
            while (inp->cursor > 0 && (inp->buf[inp->cursor] & 0xC0) == 0x80)
                inp->cursor--;
        }
        return false;
    }
    if (kd->key == EZT_KEY_RIGHT) {
        if (inp->cursor < inp->len) {
            inp->cursor++;
            while (inp->cursor < inp->len && (inp->buf[inp->cursor] & 0xC0) == 0x80)
                inp->cursor++;
        }
        return false;
    }
    if (kd->key == EZT_KEY_HOME) { inp->cursor = 0;        return false; }
    if (kd->key == EZT_KEY_END)  { inp->cursor = inp->len; return false; }

    if (kd->key < 0x20 || kd->key == 0x7F) return false;
    if (kd->mods & (EZT_MOD_CTRL | EZT_MOD_ALT)) return false;

    char enc[4];
    int n = utf8_encode(kd->key, enc);
    if (inp->len + n >= inp->buf_size) return false;
    memmove(inp->buf + inp->cursor + n,
            inp->buf + inp->cursor,
            (size_t)(inp->len - inp->cursor));
    memcpy(inp->buf + inp->cursor, enc, (size_t)n);
    inp->cursor += n; inp->len += n;
    inp->buf[inp->len] = '\0';
    return false;
}

void ezt_input_draw(const ezt_input_t *inp,
                    int x, int y, int w, ezt_style_t style) {
    int cursor_col   = utf8_width_n(inp->buf, inp->cursor);
    int view_start   = 0;
    if (cursor_col >= w) view_start = cursor_col - w + 1;

    ezt_move(x, y);
    ezt_color(style.fg, style.bg);
    ezt_attr(style.attr);

    const char *p = inp->buf;
    int col = 0, drawn = 0;
    uint32_t cp;
    while (*p && drawn < w) {
        cp = utf8_decode(&p);
        int cw = ucp_width(cp);
        if (col >= view_start && col + cw <= view_start + w) {
            ezt_putc(cp);
            drawn += cw;
        }
        col += cw;
    }
    for (int i = drawn; i < w; i++) ezt_putc(' ');
}

/* =========================================================================
 * List
 * ========================================================================= */
void ezt_list_init(ezt_list_t *lst, const char **items, int count) {
    lst->items    = items;
    lst->count    = count;
    lst->selected = 0;
    lst->scroll   = 0;
}

bool ezt_list_feed(ezt_list_t *lst, const ezt_key_data_t *kd) {
    if (kd->key == EZT_KEY_UP   && lst->selected > 0)             { lst->selected--; return true; }
    if (kd->key == EZT_KEY_DOWN && lst->selected < lst->count - 1){ lst->selected++; return true; }
    if (kd->key == EZT_KEY_ENTER) return true;
    return false;
}

void ezt_list_draw(const ezt_list_t *lst,
                   int x, int y, int w, int h,
                   ezt_style_t normal, ezt_style_t selected) {
    int scroll = lst->scroll;
    if (lst->selected < scroll) scroll = lst->selected;
    if (lst->selected >= scroll + h) scroll = lst->selected - h + 1;
    ((ezt_list_t *)lst)->scroll = scroll;

    for (int i = 0; i < h; i++) {
        int idx = scroll + i;
        bool sel = (idx == lst->selected);
        ezt_style_t s = sel ? selected : normal;
        ezt_move(x, y + i);
        ezt_color(s.fg, s.bg);
        ezt_attr(s.attr);
        if (idx < lst->count) {
            int iw = utf8_width(lst->items[idx]);
            ezt_puts(lst->items[idx]);
            for (int p2 = iw; p2 < w; p2++) ezt_putc(' ');
        } else {
            for (int j = 0; j < w; j++) ezt_putc(' ');
        }
    }
}
