/* image.c — image rendering (Halfblock / Quadrant) */

#include "eztui.h"

static ezt_img_backend_t s_img_backend = EZT_IMG_HALFBLOCK;

ezt_img_backend_t ezt_img_backend(void) {
    return s_img_backend;
}

void ezt_img_set_backend(ezt_img_backend_t backend) {
    s_img_backend = backend;
}

/* -------------------------------------------------------------------------
 * Half-block renderer (▀ U+2580)
 * Each terminal cell = 1 col × 2 pixel rows.
 * ------------------------------------------------------------------------- */
static void render_halfblock(int x, int y,
                              const uint8_t *rgba, int w, int h) {
    int rows = (h + 1) / 2;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < w; col++) {
            int py_top = row * 2;
            int py_bot = py_top + 1;

            const uint8_t *tp = rgba + (py_top * w + col) * 4;
            uint32_t fg = EZT_COLOR_RGB(tp[0], tp[1], tp[2]);

            uint32_t bg = EZT_COLOR_DEFAULT;
            if (py_bot < h) {
                const uint8_t *bp = rgba + (py_bot * w + col) * 4;
                bg = EZT_COLOR_RGB(bp[0], bp[1], bp[2]);
            }

            ezt_move(x + col, y + row);
            ezt_color(fg, bg);
            ezt_attr(EZT_ATTR_NONE);
            ezt_putc(0x2580); /* ▀ */
        }
    }
}

/* -------------------------------------------------------------------------
 * Quadrant renderer (U+2580–U+259F)
 * Each terminal cell = 2 cols × 2 pixel rows = 4 sub-pixels.
 * Universally supported — all characters are in the basic Block Elements range.
 *
 * Bit layout:
 *   bit0=top-left  bit1=top-right
 *   bit2=bot-left  bit3=bot-right
 * ------------------------------------------------------------------------- */
static const uint32_t s_quadrant[16] = {
    0x0020,  /* 0000 space        */
    0x2598,  /* 0001 ▘ top-left   */
    0x259D,  /* 0010 ▝ top-right  */
    0x2580,  /* 0011 ▀ top half   */
    0x2596,  /* 0100 ▖ bot-left   */
    0x258C,  /* 0101 ▌ left half  */
    0x259E,  /* 0110 ▞ diagonal   */
    0x259B,  /* 0111 ▛            */
    0x2597,  /* 1000 ▗ bot-right  */
    0x259A,  /* 1001 ▚ diagonal   */
    0x2590,  /* 1010 ▐ right half */
    0x259C,  /* 1011 ▜            */
    0x2584,  /* 1100 ▄ bot half   */
    0x2599,  /* 1101 ▙            */
    0x259F,  /* 1110 ▟            */
    0x2588,  /* 1111 █ full block */
};

static void render_quadrant(int x, int y,
                             const uint8_t *rgba, int w, int h) {
    int cell_cols = (w + 1) / 2;
    int cell_rows = (h + 1) / 2;

    for (int crow = 0; crow < cell_rows; crow++) {
        for (int ccol = 0; ccol < cell_cols; ccol++) {

            /* bit0=TL bit1=TR bit2=BL bit3=BR */
            int px[4] = { ccol*2,   ccol*2+1, ccol*2,   ccol*2+1 };
            int py[4] = { crow*2,   crow*2,   crow*2+1, crow*2+1 };

            uint8_t pr[4], pg[4], pb[4];
            int total_luma = 0, n_valid = 0;
            for (int i = 0; i < 4; i++) {
                if (px[i] < w && py[i] < h) {
                    const uint8_t *p = rgba + (py[i] * w + px[i]) * 4;
                    pr[i] = p[0]; pg[i] = p[1]; pb[i] = p[2];
                    // total_luma += (int)pr[i]*299 + (int)pg[i]*587 + (int)pb[i]*114;
                    total_luma += (pr[i] * 218 + pg[i] * 732 + pb[i] * 74);
                    n_valid++;
                } else {
                    pr[i] = pg[i] = pb[i] = 0;
                }
            }
            if (n_valid == 0) continue;
            int avg_luma = total_luma / n_valid;

            uint8_t pattern = 0;
            int fg_r = 0, fg_g = 0, fg_b = 0, fg_n = 0;
            int bg_r = 0, bg_g = 0, bg_b = 0, bg_n = 0;

            for (int i = 0; i < 4; i++) {
                if (px[i] >= w || py[i] >= h) continue;
                int luma = (int)pr[i]*299 + (int)pg[i]*587 + (int)pb[i]*114;
                if (luma >= avg_luma) {
                    pattern |= (uint8_t)(1 << i);
                    fg_r += pr[i]; fg_g += pg[i]; fg_b += pb[i]; fg_n++;
                } else {
                    bg_r += pr[i]; bg_g += pg[i]; bg_b += pb[i]; bg_n++;
                }
            }

            uint32_t fg_color = fg_n > 0
                ? EZT_COLOR_RGB(fg_r/fg_n, fg_g/fg_n, fg_b/fg_n)
                : EZT_COLOR_DEFAULT;
            uint32_t bg_color = bg_n > 0
                ? EZT_COLOR_RGB(bg_r/bg_n, bg_g/bg_n, bg_b/bg_n)
                : EZT_COLOR_DEFAULT;

            if (fg_n == 0) { fg_color = bg_color; pattern = 0;  }
            if (bg_n == 0) { bg_color = fg_color; pattern = 15; }

            // if (fg_n > 0 && bg_n > 0) {
            //     int diff = abs(fg_r/fg_n - bg_r/bg_n)
            //              + abs(fg_g/fg_n - bg_g/bg_n)
            //              + abs(fg_b/fg_n - bg_b/bg_n);
            //     if (diff < 12) {
            //         int n = fg_n + bg_n;
            //         fg_color = bg_color = EZT_COLOR_RGB(
            //             (fg_r+bg_r)/n, (fg_g+bg_g)/n, (fg_b+bg_b)/n);
            //         pattern = 15;
            //     }
            // }

            ezt_move(x + ccol, y + crow);
            ezt_color(fg_color, bg_color);
            ezt_attr(EZT_ATTR_NONE);
            ezt_putc(s_quadrant[pattern]);
        }
    }
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */
void ezt_image(int x, int y, const uint8_t *rgba, int w, int h) {
      
    switch (s_img_backend)
    {
    case EZT_IMG_QUADRANT:
        render_quadrant(x, y, rgba, w, h);
        break;
    case EZT_IMG_HALFBLOCK:
    default:
        render_halfblock(x, y, rgba, w, h);
        break;
    }
}
