/* input.c — raw input parsing → emit events via pub/sub */

#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "term_internal.h"
#include "unicode_internal.h"
#include "eztui.h"

/* -------------------------------------------------------------------------
 * Raw byte queue
 * ------------------------------------------------------------------------- */
#define IBUF_SIZE 256
static char s_ibuf[IBUF_SIZE];
static int  s_ibuf_len;
static int  s_ibuf_pos;

static int           ibuf_avail(void) { return s_ibuf_len - s_ibuf_pos; }
static unsigned char ibuf_peek(int off){ return (unsigned char)s_ibuf[s_ibuf_pos + off]; }
static void          ibuf_consume(int n){ s_ibuf_pos += n; }

static void ibuf_refill(int timeout_ms) {
    int rem = ibuf_avail();
    if (rem > 0 && s_ibuf_pos > 0)
        memmove(s_ibuf, s_ibuf + s_ibuf_pos, (size_t)rem);
    s_ibuf_pos = 0;
    s_ibuf_len = rem;
    int n = term_platform_read(s_ibuf + s_ibuf_len,
                               IBUF_SIZE - s_ibuf_len,
                               timeout_ms);
    if (n > 0) s_ibuf_len += n;
}

/* -------------------------------------------------------------------------
 * ESC / CSI parser → fill ezt_key_data_t or ezt_mouse_data_t
 * ------------------------------------------------------------------------- */
static bool parse_csi(ezt_key_data_t *kd, ezt_mouse_data_t *md,
                      bool *is_mouse) {
    if (ibuf_avail() < 1) return false;
    char buf[32]; int bi = 0;

    for (int i = 0; i < ibuf_avail() && bi < 30; i++) {
        char c = (char)ibuf_peek(i);
        buf[bi++] = c;
        if (c >= 0x40 && c <= 0x7E) {
            ibuf_consume(bi);
            buf[bi] = '\0';

            /* SGR mouse: ESC [ < Pb ; Px ; Py M/m */
            if (buf[0] == '<') {
                int btn, x, y;
                char final_c = buf[bi-1];
                buf[bi-1] = '\0';
                if (sscanf(buf+1, "%d;%d;%d", &btn, &x, &y) == 3) {
                    *is_mouse  = true;
                    md->x      = x - 1;
                    md->y      = y - 1;
                    md->buttons = 0;
                    int b = btn & 3;
                    if (b == 0) md->buttons |= EZT_MB_LEFT;
                    else if (b == 1) md->buttons |= EZT_MB_MIDDLE;
                    else if (b == 2) md->buttons |= EZT_MB_RIGHT;
                    if (btn & 64)
                        md->buttons = (btn & 1) ? EZT_MB_SCROLL_DOWN
                                                 : EZT_MB_SCROLL_UP;
                    md->mods = 0;
                    if (btn & 4)  md->mods |= EZT_MOD_SHIFT;
                    if (btn & 8)  md->mods |= EZT_MOD_ALT;
                    if (btn & 16) md->mods |= EZT_MOD_CTRL;
                    (void)final_c;
                    return true;
                }
                return false;
            }

            /* Arrow / nav */
            if (bi == 1) {
                kd->mods = 0;
                switch (c) {
                    case 'A': kd->key = EZT_KEY_UP;    return true;
                    case 'B': kd->key = EZT_KEY_DOWN;  return true;
                    case 'C': kd->key = EZT_KEY_RIGHT; return true;
                    case 'D': kd->key = EZT_KEY_LEFT;  return true;
                    case 'H': kd->key = EZT_KEY_HOME;  return true;
                    case 'F': kd->key = EZT_KEY_END;   return true;
                    default: break;
                }
            }
            /* ~-terminated */
            if (c == '~' && bi >= 2) {
                int n = atoi(buf);
                kd->mods = 0;
                switch (n) {
                    case 1:  kd->key = EZT_KEY_HOME;   return true;
                    case 2:  kd->key = EZT_KEY_INSERT; return true;
                    case 3:  kd->key = EZT_KEY_DELETE; return true;
                    case 4:  kd->key = EZT_KEY_END;    return true;
                    case 5:  kd->key = EZT_KEY_PGUP;   return true;
                    case 6:  kd->key = EZT_KEY_PGDN;   return true;
                    case 11: kd->key = EZT_KEY_F(1);   return true;
                    case 12: kd->key = EZT_KEY_F(2);   return true;
                    case 13: kd->key = EZT_KEY_F(3);   return true;
                    case 14: kd->key = EZT_KEY_F(4);   return true;
                    case 15: kd->key = EZT_KEY_F(5);   return true;
                    case 17: kd->key = EZT_KEY_F(6);   return true;
                    case 18: kd->key = EZT_KEY_F(7);   return true;
                    case 19: kd->key = EZT_KEY_F(8);   return true;
                    case 20: kd->key = EZT_KEY_F(9);   return true;
                    case 21: kd->key = EZT_KEY_F(10);  return true;
                    case 23: kd->key = EZT_KEY_F(11);  return true;
                    case 24: kd->key = EZT_KEY_F(12);  return true;
                    default: break;
                }
            }
            return false;
        }
    }
    return false;
}

static bool parse_esc(ezt_key_data_t *kd, ezt_mouse_data_t *md,
                      bool *is_mouse, bool *is_resize) {
    (void)is_resize;
    if (ibuf_avail() == 0) {
        kd->key = EZT_KEY_ESC; kd->mods = 0; return true;
    }
    unsigned char next = ibuf_peek(0);
    if (next == '[') {
        ibuf_consume(1);
        return parse_csi(kd, md, is_mouse);
    }
    if (next == 'O') {
        ibuf_consume(1);
        if (ibuf_avail() < 1) return false;
        char c = (char)ibuf_peek(0); ibuf_consume(1);
        kd->mods = 0;
        switch (c) {
            case 'P': kd->key = EZT_KEY_F(1);  return true;
            case 'Q': kd->key = EZT_KEY_F(2);  return true;
            case 'R': kd->key = EZT_KEY_F(3);  return true;
            case 'S': kd->key = EZT_KEY_F(4);  return true;
            case 'H': kd->key = EZT_KEY_HOME;  return true;
            case 'F': kd->key = EZT_KEY_END;   return true;
            default:  kd->key = EZT_KEY_ESC;   return true;
        }
    }
    /* Alt + key */
    kd->key  = (unsigned char)ibuf_peek(0);
    kd->mods = EZT_MOD_ALT;
    ibuf_consume(1);
    return true;
}

/* -------------------------------------------------------------------------
 * Read one raw event and emit it.  Returns true if something was emitted.
 * ------------------------------------------------------------------------- */
bool input_read_and_emit(int timeout_ms) {
    if (ibuf_avail() == 0) ibuf_refill(timeout_ms);
    if (ibuf_avail() == 0) return false;

    unsigned char c = ibuf_peek(0);

    /* Resize sentinel */
    if (c == 0xFF) {
        ibuf_consume(1);
        int w, h;
        term_platform_get_size(&w, &h);
        ezt_resize(w, h);
        ezt_resize_data_t rd = { w, h };
        ezt_emit(EZT_EV_RESIZE, &rd);
        return true;
    }

    ezt_key_data_t   kd = {0};
    ezt_mouse_data_t md = {0};
    bool is_mouse  = false;
    bool is_resize = false;

    if (c == 0x1B) {
        ibuf_consume(1);
        if (ibuf_avail() == 0) ibuf_refill(20);
        if (!parse_esc(&kd, &md, &is_mouse, &is_resize)) return false;
    } else {
        ibuf_consume(1);
        kd.mods = 0;
        switch (c) {
            case '\r': case '\n': kd.key = EZT_KEY_ENTER;     break;
            case '\t':            kd.key = EZT_KEY_TAB;       break;
            case 127:             kd.key = EZT_KEY_BACKSPACE;  break;
            default:
                if (c >= 0x01 && c <= 0x1A) {
                    kd.key  = 'a' + c - 1;
                    kd.mods = EZT_MOD_CTRL;
                } else {
                    /* UTF-8 */
                    s_ibuf_pos--;
                    const char *p  = s_ibuf + s_ibuf_pos;
                    const char *p0 = p;
                    kd.key = utf8_decode(&p);
                    s_ibuf_pos += (int)(p - p0);
                }
                break;
        }
    }

    if (is_mouse)
        ezt_emit(EZT_EV_MOUSE, &md);
    else
        ezt_emit(EZT_EV_KEY, &kd);

    return true;
}
