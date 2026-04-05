/* color.c — color capability detection */

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include "color_internal.h"

static ezt_color_cap_t s_cap = EZT_COLORCAP_UNKNOWN;

ezt_color_cap_t color_detect(void) {
    if (s_cap != EZT_COLORCAP_UNKNOWN) return s_cap;

    const char *colorterm = getenv("COLORTERM");
    if (colorterm) {
        if (strcmp(colorterm, "truecolor") == 0 ||
            strcmp(colorterm, "24bit") == 0) {
            s_cap = EZT_COLORCAP_TRUECOLOR;
            return s_cap;
        }
    }

    const char *term = getenv("TERM");
    const char *term_program = getenv("TERM_PROGRAM");

    if (term_program) {
        if (strcmp(term_program, "iTerm.app") == 0 ||
            strcmp(term_program, "WezTerm") == 0) {
            s_cap = EZT_COLORCAP_TRUECOLOR;
            return s_cap;
        }
    }

    if (term) {
        if (strstr(term, "256color")) {
            s_cap = EZT_COLORCAP_256;
            return s_cap;
        }
        if (strstr(term, "color") || strstr(term, "xterm") ||
            strstr(term, "screen") || strstr(term, "tmux")) {
            s_cap = EZT_COLORCAP_16;
            return s_cap;
        }
    }

    s_cap = EZT_COLORCAP_NONE;
    return s_cap;
}

/* Downgrade a color to the detected capability level */
uint32_t color_downgrade(uint32_t color, ezt_color_cap_t cap) {
    if (color == EZT_COLOR_DEFAULT) return color;

    uint32_t tag = color >> 24;

    /* Already within capability */
    if (cap == EZT_COLORCAP_TRUECOLOR) return color;

    if (cap == EZT_COLORCAP_256) {
        /* 16-color and 256-color pass through; truecolor → 256 approximation */
        if (tag == 0xFE || tag == 0xFD) return color;
        /* Approximate truecolor → 256 */
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >>  8) & 0xFF;
        uint8_t b =  color        & 0xFF;
        /* Map to 6x6x6 color cube (indices 16..231) */
        uint8_t ri = (uint8_t)((r * 5 + 127) / 255);
        uint8_t gi = (uint8_t)((g * 5 + 127) / 255);
        uint8_t bi = (uint8_t)((b * 5 + 127) / 255);
        return EZT_COLOR_256(16 + 36 * ri + 6 * gi + bi);
    }

    if (cap == EZT_COLORCAP_16) {
        if (tag == 0xFE) return color; /* already 16-color */
        /* 256-color → 16-color: indices 0..15 map directly */
        if (tag == 0xFD) {
            uint32_t idx = color & 0xFF;
            if (idx < 16) return EZT_COLOR_16(idx);
        }
        /* Truecolor → 16: crude luminance-based approximation */
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >>  8) & 0xFF;
        uint8_t b =  color        & 0xFF;
        int bright = (r > 127 || g > 127 || b > 127) ? 8 : 0;
        int ri = r > 64 ? 1 : 0;
        int gi = g > 64 ? 1 : 0;
        int bi = b > 64 ? 1 : 0;
        return EZT_COLOR_16((uint32_t)(bright | (ri) | (gi << 1) | (bi << 2)));
    }

    return EZT_COLOR_DEFAULT;
}
