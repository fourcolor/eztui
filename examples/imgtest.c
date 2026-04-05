/* imgtest.c — image viewer with zoom/pan, demonstrates ezt_image() */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "eztui.h"

/* -------------------------------------------------------------------------
 * Bilinear resize
 * ------------------------------------------------------------------------- */
static uint8_t *img_resize(const uint8_t *src, int sw, int sh,
                            int dw, int dh) {
    uint8_t *dst = malloc((size_t)(dw * dh * 4));
    if (!dst) return NULL;
    for (int dy = 0; dy < dh; dy++) {
        float fy = (float)dy * (sh - 1) / (dh > 1 ? dh - 1 : 1);
        int   y0 = (int)fy;
        int   y1 = y0 + 1 < sh ? y0 + 1 : y0;
        float vy = fy - y0;
        for (int dx = 0; dx < dw; dx++) {
            float fx = (float)dx * (sw - 1) / (dw > 1 ? dw - 1 : 1);
            int   x0 = (int)fx;
            int   x1 = x0 + 1 < sw ? x0 + 1 : x0;
            float vx = fx - x0;
            for (int c = 0; c < 4; c++) {
                float v = (1 - vx) * (1 - vy) * src[(y0 * sw + x0) * 4 + c]
                        + (    vx) * (1 - vy) * src[(y0 * sw + x1) * 4 + c]
                        + (1 - vx) * (    vy) * src[(y1 * sw + x0) * 4 + c]
                        + (    vx) * (    vy) * src[(y1 * sw + x1) * 4 + c];
                dst[(dy * dw + dx) * 4 + c] = (uint8_t)(v + 0.5f);
            }
        }
    }
    return dst;
}

/* -------------------------------------------------------------------------
 * App state
 * ------------------------------------------------------------------------- */
static uint8_t *s_orig = NULL;
static int      s_orig_w, s_orig_h;
static float    s_zoom  = 1.0f;
static int      s_pan_x = 0, s_pan_y = 0;
static bool     s_running = true;
static bool     s_dirty   = true;

/* Backend sub-pixel ratios (pixels per terminal cell) */
static int s_sub_w = 1, s_sub_h = 2;

static void init_sub_ratio(void) {
    switch (ezt_img_backend()) {
        case EZT_IMG_QUADRANT: s_sub_w = 2; s_sub_h = 2; break;
        default:               s_sub_w = 1; s_sub_h = 2; break;
    }
}

static float cell_sub_aspect(void) {
    /* sub-pixel h/w ratio = (cell_h / s_sub_h) / (cell_w / s_sub_w)
     *                     = aspect * s_sub_w / s_sub_h
     * Halfblock: 2 * 1/2 = 1.0  (square sub-pixel, no correction needed)
     * FIXME: Quadrant's sub-pixel size is a assumption
     * Quadrant:  2 * 2/2 = 2.0  (sub-pixel is 2x taller than wide) */
    return ezt_cell_aspect() * (float)s_sub_w / (float)s_sub_h;
}

static void fit_to_screen(void) {
    int sw, sh;
    ezt_get_size(&sw, &sh);
    float sa = cell_sub_aspect();
    float scale_x = (float)(sw * s_sub_w) / s_orig_w;
    float scale_y = (float)((sh - 1) * s_sub_h) * sa / s_orig_h;
    s_zoom  = scale_x < scale_y ? scale_x : scale_y;
    s_pan_x = 0;
    s_pan_y = 0;
}

static void draw(void) {
    int sw, sh;
    ezt_get_size(&sw, &sh);

    int pw = (int)(s_orig_w * s_zoom);
    int ph = (int)(s_orig_h * s_zoom / cell_sub_aspect());
    if (pw < 1) pw = 1;
    if (ph < 1) ph = 1;

    int cw = (pw + s_sub_w - 1) / s_sub_w;
    int ch = (ph + s_sub_h - 1) / s_sub_h;

    int max_px = sw - cw; if (max_px < 0) max_px = 0;
    int max_py = (sh - 1) - ch; if (max_py < 0) max_py = 0;
    if (s_pan_x > max_px) s_pan_x = max_px;
    if (s_pan_y > max_py) s_pan_y = max_py;
    if (s_pan_x < 0) s_pan_x = 0;
    if (s_pan_y < 0) s_pan_y = 0;

    uint8_t *scaled = img_resize(s_orig, s_orig_w, s_orig_h, pw, ph);
    if (!scaled) return;

    ezt_clear();
    ezt_image(s_pan_x, s_pan_y, scaled, pw, ph);
    free(scaled);

    const char *bname;
    switch (ezt_img_backend()) {
        case EZT_IMG_QUADRANT: bname = "quadrant"; break;
        default:               bname = "halfblock"; break;
    }

    char hud[128];
    snprintf(hud, sizeof(hud),
             " %s  zoom:%.0f%%  +/-:zoom  0:fit  arrows/hjkl:pan  Q:quit ",
             bname, (double)(s_zoom * 100.0f));
    ezt_draw_text(0, sh - 1, hud,
                  EZT_STYLE(EZT_BLACK, EZT_BRIGHT_WHITE, EZT_ATTR_NONE));

    ezt_refresh();
    s_dirty = false;
}

/* -------------------------------------------------------------------------
 * Event handlers
 * ------------------------------------------------------------------------- */
static void on_key(const ezt_event_t *ev, void *ud) {
    (void)ud;
    const ezt_key_data_t *kd = ev->data;
    switch (kd->key) {
        case 'q': case 'Q': case EZT_KEY_ESC:
            s_running = false; break;
        case '+': case '=':
            s_zoom *= 1.25f; s_dirty = true; break;
        case '-':
            s_zoom /= 1.25f;
            if (s_zoom < 0.05f) s_zoom = 0.05f;
            s_dirty = true; break;
        case '0':
            fit_to_screen(); s_dirty = true; break;
        case EZT_KEY_LEFT:  case 'h': s_pan_x -= 2; s_dirty = true; break;
        case EZT_KEY_RIGHT: case 'l': s_pan_x += 2; s_dirty = true; break;
        case EZT_KEY_UP:    case 'k': s_pan_y -= 1; s_dirty = true; break;
        case EZT_KEY_DOWN:  case 'j': s_pan_y += 1; s_dirty = true; break;
        default: break;
    }
}

static void on_resize(const ezt_event_t *ev, void *ud) {
    (void)ev; (void)ud;
    s_dirty = true;
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(int argc, char **argv) {
    const char *path = "pic.png";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--halfblock") == 0)
            ezt_img_set_backend(EZT_IMG_HALFBLOCK);
        else if (strcmp(argv[i], "--quadrant") == 0)
            ezt_img_set_backend(EZT_IMG_QUADRANT);
        else
            path = argv[i];
    }

    int w, h, n;
    uint8_t *data = stbi_load(path, &w, &h, &n, 4);
    if (!data) {
        fprintf(stderr, "Failed to load image: %s\n", path);
        return 1;
    }
    s_orig   = data;
    s_orig_w = w;
    s_orig_h = h;

    if (ezt_init() < 0) { stbi_image_free(data); return 1; }

    init_sub_ratio();
    fit_to_screen();

    ezt_sub_t *ksub = ezt_on(EZT_EV_KEY,    on_key,    NULL);
    ezt_sub_t *rsub = ezt_on(EZT_EV_RESIZE, on_resize, NULL);

    while (s_running) {
        if (s_dirty) draw();
        ezt_tick(100);
    }

    ezt_off(ksub);
    ezt_off(rsub);
    ezt_cleanup();
    stbi_image_free(data);
    return 0;
}
