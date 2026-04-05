#pragma once

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * eztui  —  lightweight C11 TUI library
 * ========================================================================= */

/* -------------------------------------------------------------------------
 * Color
 *   EZT_COLOR_DEFAULT       terminal default
 *   EZT_COLOR_16(n)         standard 16-color palette  (n = 0..15)
 *   EZT_COLOR_256(n)        256-color palette           (n = 0..255)
 *   EZT_COLOR_RGB(r,g,b)    24-bit truecolor
 * ------------------------------------------------------------------------- */
#define EZT_COLOR_DEFAULT    UINT32_C(0xFF000000)
#define EZT_COLOR_16(n)      (UINT32_C(0xFE000000) | (uint32_t)(n))
#define EZT_COLOR_256(n)     (UINT32_C(0xFD000000) | (uint32_t)(n))
#define EZT_COLOR_RGB(r,g,b) (((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|(uint32_t)(b))

#define EZT_BLACK            EZT_COLOR_16(0)
#define EZT_RED              EZT_COLOR_16(1)
#define EZT_GREEN            EZT_COLOR_16(2)
#define EZT_YELLOW           EZT_COLOR_16(3)
#define EZT_BLUE             EZT_COLOR_16(4)
#define EZT_MAGENTA          EZT_COLOR_16(5)
#define EZT_CYAN             EZT_COLOR_16(6)
#define EZT_WHITE            EZT_COLOR_16(7)
#define EZT_BRIGHT_BLACK     EZT_COLOR_16(8)
#define EZT_BRIGHT_RED       EZT_COLOR_16(9)
#define EZT_BRIGHT_GREEN     EZT_COLOR_16(10)
#define EZT_BRIGHT_YELLOW    EZT_COLOR_16(11)
#define EZT_BRIGHT_BLUE      EZT_COLOR_16(12)
#define EZT_BRIGHT_MAGENTA   EZT_COLOR_16(13)
#define EZT_BRIGHT_CYAN      EZT_COLOR_16(14)
#define EZT_BRIGHT_WHITE     EZT_COLOR_16(15)

/* -------------------------------------------------------------------------
 * Text attributes (bitmask)
 * ------------------------------------------------------------------------- */
#define EZT_ATTR_NONE          0
#define EZT_ATTR_BOLD          (1<<0)
#define EZT_ATTR_DIM           (1<<1)
#define EZT_ATTR_ITALIC        (1<<2)
#define EZT_ATTR_UNDERLINE     (1<<3)
#define EZT_ATTR_BLINK         (1<<4)
#define EZT_ATTR_REVERSE       (1<<5)
#define EZT_ATTR_STRIKETHROUGH (1<<6)

/* -------------------------------------------------------------------------
 * Style  —  bundles fg, bg, attr into one value
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t fg, bg;
    int      attr;
} ezt_style_t;

#define EZT_STYLE(fg, bg, attr) ((ezt_style_t){(fg), (bg), (attr)})
#define EZT_STYLE_DEFAULT       EZT_STYLE(EZT_COLOR_DEFAULT, EZT_COLOR_DEFAULT, EZT_ATTR_NONE)

/* -------------------------------------------------------------------------
 * Rect
 * ------------------------------------------------------------------------- */
typedef struct {
    int x, y, w, h;
} ezt_rect_t;

#define EZT_RECT(x,y,w,h) ((ezt_rect_t){(x),(y),(w),(h)})

/* -------------------------------------------------------------------------
 * Built-in event IDs
 * ------------------------------------------------------------------------- */
#define EZT_EV_KEY     UINT32_C(1)
#define EZT_EV_MOUSE   UINT32_C(2)
#define EZT_EV_RESIZE  UINT32_C(3)
#define EZT_EV_TIMER   UINT32_C(4)
#define EZT_EV_FOCUS   UINT32_C(5)
#define EZT_EV_BLUR    UINT32_C(6)
/* User-registered IDs start at EZT_EV_USER */
#define EZT_EV_USER    UINT32_C(0x1000)

/* -------------------------------------------------------------------------
 * Event data structs  (cast ev->data to these in handlers)
 * ------------------------------------------------------------------------- */

/* Special key codes */
#define EZT_KEY_UP        0x110000
#define EZT_KEY_DOWN      0x110001
#define EZT_KEY_LEFT      0x110002
#define EZT_KEY_RIGHT     0x110003
#define EZT_KEY_HOME      0x110004
#define EZT_KEY_END       0x110005
#define EZT_KEY_PGUP      0x110006
#define EZT_KEY_PGDN      0x110007
#define EZT_KEY_INSERT    0x110008
#define EZT_KEY_DELETE    0x110009
#define EZT_KEY_F(n)      (0x110010 + (n))
#define EZT_KEY_BACKSPACE 0x110020
#define EZT_KEY_TAB       0x110021
#define EZT_KEY_ENTER     0x110022
#define EZT_KEY_ESC       0x110023

#define EZT_MOD_SHIFT (1<<0)
#define EZT_MOD_CTRL  (1<<1)
#define EZT_MOD_ALT   (1<<2)

#define EZT_MB_LEFT        (1<<0)
#define EZT_MB_MIDDLE      (1<<1)
#define EZT_MB_RIGHT       (1<<2)
#define EZT_MB_SCROLL_UP   (1<<3)
#define EZT_MB_SCROLL_DOWN (1<<4)

typedef struct { uint32_t key; int mods; }        ezt_key_data_t;
typedef struct { int x, y, buttons, mods; }       ezt_mouse_data_t;
typedef struct { int w, h; }                       ezt_resize_data_t;
typedef struct { void *userdata; }                 ezt_timer_data_t;
/* EZT_EV_FOCUS / EZT_EV_BLUR: data = (ezt_node_t *) that gained/lost focus */

/* -------------------------------------------------------------------------
 * Event  —  passed to every handler
 * ------------------------------------------------------------------------- */
typedef struct {
    uint32_t  id;
    void     *data;   /* valid only during handler execution */
} ezt_event_t;

/* -------------------------------------------------------------------------
 * Event system  —  pure pub/sub, independent of node tree
 * ------------------------------------------------------------------------- */
typedef struct ezt_sub ezt_sub_t;  /* opaque */
typedef void (*ezt_handler_fn)(const ezt_event_t *ev, void *userdata);

/* Subscribe to an event type globally.
 * Returns a token needed for unsubscription. */
ezt_sub_t *ezt_on (uint32_t ev_id, ezt_handler_fn fn, void *userdata);

/* Unsubscribe.  Safe to call with NULL. */
void       ezt_off(ezt_sub_t *sub);

/* Emit an event — synchronously notifies all subscribers in FIFO order.
 * data is only valid during this call. */
void ezt_emit(uint32_t ev_id, void *data);

/* Register a custom event type. Returns a unique ID >= EZT_EV_USER. */
uint32_t ezt_event_register(const char *name);

/* -------------------------------------------------------------------------
 * Component  —  a renderable item with a layer
 * layer: render order within its container (lower = behind, higher = in front)
 * ------------------------------------------------------------------------- */
typedef struct ezt_comp ezt_comp_t;

struct ezt_comp {
    int         layer;
    ezt_rect_t  rect;       /* absolute position/size                    */
    ezt_rect_t  hint;       /* layout hint: w=0/h=0 means flex           */
    ezt_style_t style;
    bool        focusable;
    void       *data;
    void      (*render) (ezt_comp_t *comp, ezt_rect_t rect);
    void      (*destroy)(ezt_comp_t *comp);
};

ezt_comp_t *ezt_comp_new (int layer,
                           void (*render)(ezt_comp_t *, ezt_rect_t),
                           void *data);
void        ezt_comp_free(ezt_comp_t *comp);

/* -------------------------------------------------------------------------
 * Group  —  a container with its own layer and optional auto-layout
 * Children (comps and nested groups) are rendered sorted by their layer.
 * Within the same layer, items are rendered in insertion order.
 * The group's own layer is used when comparing against siblings.
 * ------------------------------------------------------------------------- */
typedef struct ezt_group ezt_group_t;
typedef void (*ezt_layout_fn)(ezt_group_t *group);

struct ezt_group {
    int            layer;
    ezt_rect_t     rect;        /* absolute position/size               */
    ezt_rect_t     hint;        /* layout hint for parent               */
    ezt_layout_fn  layout_fn;   /* NULL = manual (user sets rects)      */
    void          *data;
    void         (*destroy)(ezt_group_t *group);
    /* private */
    void          *_items;
    int            _count, _cap, _ins;
};

ezt_group_t *ezt_group_new      (int layer);
void         ezt_group_free     (ezt_group_t *group);   /* does not free children */
void         ezt_group_add_comp (ezt_group_t *group, ezt_comp_t  *comp);
void         ezt_group_add_group(ezt_group_t *group, ezt_group_t *child);
/* Run layout_fn on this group then recurse into child groups. */
void         ezt_group_layout   (ezt_group_t *group);

/* Built-in layout functions */
void ezt_layout_vbox(ezt_group_t *group);   /* stack children vertically   */
void ezt_layout_hbox(ezt_group_t *group);   /* stack children horizontally */

/* -------------------------------------------------------------------------
 * Scene  —  global top-level container
 * Items are rendered sorted by layer (ties: insertion order).
 * Comparing a standalone comp vs a group uses each item's own layer.
 * ------------------------------------------------------------------------- */
void ezt_scene_add_comp    (ezt_comp_t  *comp);
void ezt_scene_add_group   (ezt_group_t *group);
void ezt_scene_remove_comp (ezt_comp_t  *comp);
void ezt_scene_remove_group(ezt_group_t *group);
void ezt_scene_clear       (void);   /* detach all items (does not free) */
void ezt_scene_render      (void);   /* render all items by layer        */

/* -------------------------------------------------------------------------
 * Focus  (terminal has no native focus; managed entirely in software)
 * Cycles through focusable comps in scene render order (layer order).
 * ------------------------------------------------------------------------- */
void        ezt_focus     (ezt_comp_t *comp);
ezt_comp_t *ezt_focused   (void);
bool        ezt_is_focused(ezt_comp_t *comp);
void        ezt_focus_next(void);
void        ezt_focus_prev(void);

/* -------------------------------------------------------------------------
 * Timer
 * ------------------------------------------------------------------------- */
typedef struct ezt_timer ezt_timer_t;

/* Creates a timer that emits EZT_EV_TIMER every interval_ms.
 * repeat=false: fires once then auto-frees.
 * userdata is passed inside ezt_timer_data_t.userdata. */
ezt_timer_t *ezt_timer_new (int interval_ms, bool repeat, void *userdata);
void         ezt_timer_free(ezt_timer_t *t);

/* -------------------------------------------------------------------------
 * Main loop helper
 * Polls for input, fires expired timers, emits events to subscribers.
 * timeout_ms: -1 = block until event, 0 = non-blocking.
 * Returns true if at least one event was dispatched. */
bool ezt_tick(int timeout_ms);

/* -------------------------------------------------------------------------
 * Init / cleanup
 * ------------------------------------------------------------------------- */
int  ezt_init   (void);
void ezt_cleanup(void);
void  ezt_get_size   (int *w, int *h);
float ezt_cell_aspect(void);          /* cell height / cell width */
void ezt_resize (int w, int h);

/* -------------------------------------------------------------------------
 * Draw API  —  stateless, each call takes position + style
 * ------------------------------------------------------------------------- */
void ezt_draw_text (int x, int y, const char *utf8, ezt_style_t style);
void ezt_draw_fill (int x, int y, int w, int h, uint32_t cp, ezt_style_t style);

typedef enum {
    EZT_BOX_ASCII = 0,
    EZT_BOX_SINGLE,
    EZT_BOX_DOUBLE,
    EZT_BOX_ROUNDED,
    EZT_BOX_BOLD,
} ezt_box_style_t;

void ezt_draw_box (int x, int y, int w, int h,
                   ezt_box_style_t box_style, ezt_style_t style);
void ezt_draw_hbar(int x, int y, int w, ezt_style_t style);
void ezt_draw_vbar(int x, int y, int h, ezt_style_t style);

/* Screen control */
void ezt_clear  (void);
void ezt_refresh(void);

/* -------------------------------------------------------------------------
 * Low-level draw (stateful, for use inside render callbacks)
 * ------------------------------------------------------------------------- */
void ezt_move (int x, int y);
void ezt_color(uint32_t fg, uint32_t bg);
void ezt_attr (int attr_mask);
void ezt_puts (const char *utf8);
void ezt_putc (uint32_t codepoint);

/* -------------------------------------------------------------------------
 * Input widget state  (used inside node render/event callbacks)
 * ------------------------------------------------------------------------- */
typedef struct {
    char *buf;
    int   buf_size;
    int   cursor;
    int   len;
} ezt_input_t;

void ezt_input_init(ezt_input_t *inp, char *buf, int buf_size);
/* Feed a key event into input state.  Returns true when Enter pressed. */
bool ezt_input_feed(ezt_input_t *inp, const ezt_key_data_t *kd);
void ezt_input_draw(const ezt_input_t *inp,
                    int x, int y, int w, ezt_style_t style);

/* -------------------------------------------------------------------------
 * List widget state
 * ------------------------------------------------------------------------- */
typedef struct {
    const char **items;
    int          count;
    int          selected;
    int          scroll;
} ezt_list_t;

void ezt_list_init(ezt_list_t *lst, const char **items, int count);
/* Returns true when selection changes or Enter pressed. */
bool ezt_list_feed(ezt_list_t *lst, const ezt_key_data_t *kd);
void ezt_list_draw(const ezt_list_t *lst,
                   int x, int y, int w, int h,
                   ezt_style_t normal, ezt_style_t selected);

/* -------------------------------------------------------------------------
 * Image rendering
 * ------------------------------------------------------------------------- */
typedef enum {
    EZT_IMG_HALFBLOCK = 0,  /* default: ▀ U+2580, universal          */
    EZT_IMG_QUADRANT,       /* U+2580–U+259F,   2×2 sub-pixels/cell */
} ezt_img_backend_t;

ezt_img_backend_t ezt_img_backend    (void);
void              ezt_img_set_backend(ezt_img_backend_t backend);
void              ezt_image          (int x, int y,
                                      const uint8_t *rgba, int w, int h);
