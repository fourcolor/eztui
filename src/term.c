/* term.c — terminal raw mode and escape sequence output */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include "term_internal.h"

/* Write buffer for batching escape sequences */
static char  s_wbuf[65536];
static int   s_wpos;

void term_buf_flush(void) {
    if (s_wpos > 0) {
        fwrite(s_wbuf, 1, (size_t)s_wpos, stdout);
        s_wpos = 0;
    }
}

void term_buf_write(const char *data, int len) {
    if (s_wpos + len > (int)sizeof(s_wbuf))
        term_buf_flush();
    memcpy(s_wbuf + s_wpos, data, (size_t)len);
    s_wpos += len;
}

void term_buf_puts(const char *s) {
    term_buf_write(s, (int)strlen(s));
}

/* -------------------------------------------------------------------------
 * Cursor
 * ------------------------------------------------------------------------- */
void term_cursor_move(int x, int y) {
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y + 1, x + 1);
    term_buf_write(buf, n);
}

void term_cursor_hide(void) { term_buf_puts("\x1b[?25l"); }
void term_cursor_show(void) { term_buf_puts("\x1b[?25h"); }

/* -------------------------------------------------------------------------
 * Screen
 * ------------------------------------------------------------------------- */
void term_screen_clear(void)      { term_buf_puts("\x1b[2J"); }
void term_screen_alt_enter(void)  { term_buf_puts("\x1b[?1049h"); }
void term_screen_alt_leave(void)  { term_buf_puts("\x1b[?1049l"); }

/* -------------------------------------------------------------------------
 * Mouse
 * ------------------------------------------------------------------------- */
void term_mouse_enable(void) {
    term_buf_puts("\x1b[?1000h"   /* button events */
                  "\x1b[?1002h"   /* drag events   */
                  "\x1b[?1006h"); /* SGR encoding  */
}

void term_mouse_disable(void) {
    term_buf_puts("\x1b[?1006l"
                  "\x1b[?1002l"
                  "\x1b[?1000l");
}

/* -------------------------------------------------------------------------
 * SGR attributes
 * ------------------------------------------------------------------------- */
void term_sgr_reset(void) { term_buf_puts("\x1b[0m"); }

void term_sgr_attr(int attr) {
    if (attr & EZT_ATTR_BOLD)          term_buf_puts("\x1b[1m");
    if (attr & EZT_ATTR_DIM)           term_buf_puts("\x1b[2m");
    if (attr & EZT_ATTR_ITALIC)        term_buf_puts("\x1b[3m");
    if (attr & EZT_ATTR_UNDERLINE)     term_buf_puts("\x1b[4m");
    if (attr & EZT_ATTR_BLINK)         term_buf_puts("\x1b[5m");
    if (attr & EZT_ATTR_REVERSE)       term_buf_puts("\x1b[7m");
    if (attr & EZT_ATTR_STRIKETHROUGH) term_buf_puts("\x1b[9m");
}

static void write_color(uint32_t color, bool is_bg) {
    char buf[32];
    int n;
    uint32_t tag = color >> 24;

    if (color == EZT_COLOR_DEFAULT) {
        n = snprintf(buf, sizeof(buf), "\x1b[%dm", is_bg ? 49 : 39);
    } else if (tag == 0xFE) {
        /* 16-color */
        uint32_t idx = color & 0xFF;
        if (idx < 8)
            n = snprintf(buf, sizeof(buf), "\x1b[%dm", (is_bg ? 40 : 30) + idx);
        else
            n = snprintf(buf, sizeof(buf), "\x1b[%dm", (is_bg ? 100 : 90) + (idx - 8));
    } else if (tag == 0xFD) {
        /* 256-color */
        uint32_t idx = color & 0xFF;
        n = snprintf(buf, sizeof(buf), "\x1b[%d;5;%um", is_bg ? 48 : 38, idx);
    } else {
        /* 24-bit truecolor */
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >>  8) & 0xFF;
        uint8_t b =  color        & 0xFF;
        n = snprintf(buf, sizeof(buf), "\x1b[%d;2;%u;%u;%um",
                     is_bg ? 48 : 38, r, g, b);
    }
    term_buf_write(buf, n);
}

void term_sgr_fg(uint32_t color) { write_color(color, false); }
void term_sgr_bg(uint32_t color) { write_color(color, true);  }
