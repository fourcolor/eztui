# eztui

A lightweight C11 TUI library.

- Pure C11, zero external dependencies
- UTF-8 and wide character support (CJK, Emoji)
- 16-color / 256-color / 24-bit Truecolor with automatic downgrade
- Pure pub/sub event system with user-defined events
- Component / Group / Scene rendering model with layer-based ordering
- Software-managed focus
- Built-in timer
- Image rendering: Halfblock (default), Quadrant
- Linux (POSIX); Windows reserved

---

## Building

```sh
make          # build libeztui.a
make examples # build all examples into build/
make clean
```

Requirements: `cc` (gcc or clang), `make`, POSIX environment.

---

## Quick Start

```c
#include <stdbool.h>
#include <stddef.h>
#include "eztui.h"

static bool s_running = true;

static void my_render(ezt_comp_t *comp, ezt_rect_t r) {
    (void)comp;
    ezt_draw_box(r.x, r.y, r.w, r.h, EZT_BOX_ROUNDED,
                 EZT_STYLE(EZT_BRIGHT_CYAN, EZT_COLOR_DEFAULT, EZT_ATTR_NONE));
    ezt_draw_text(r.x + 2, r.y + 1, "Hello, eztui!",
                  EZT_STYLE(EZT_WHITE, EZT_COLOR_DEFAULT, EZT_ATTR_BOLD));
}

static void on_key(const ezt_event_t *ev, void *ud) {
    (void)ud;
    const ezt_key_data_t *kd = ev->data;
    if (kd->key == 'q' || kd->key == EZT_KEY_ESC)
        s_running = false;
}

int main(void) {
    if (ezt_init() < 0) return 1;

    int w, h;
    ezt_get_size(&w, &h);

    ezt_comp_t *box = ezt_comp_new(0, my_render, NULL);
    box->rect = EZT_RECT((w - 24) / 2, (h - 5) / 2, 24, 5);
    ezt_scene_add_comp(box);

    ezt_sub_t *ksub = ezt_on(EZT_EV_KEY, on_key, NULL);

    while (s_running) {
        ezt_clear();
        ezt_scene_render();
        ezt_refresh();
        ezt_tick(-1);
    }

    ezt_off(ksub);
    ezt_comp_free(box);
    ezt_cleanup();
    return 0;
}
```

Compile:

```sh
cc -std=c11 -I include main.c -L. -leztui -o myapp
```

---

## Architecture

```
┌─────────────────────────────────────────────┐
│  Application                                │
│                                             │
│  ezt_scene_add_comp / ezt_scene_add_group   │
│  ezt_scene_render()  ←  sorted by layer     │
│                                             │
│  ┌──────────┐   ┌───────────────────────┐   │
│  │ Component│   │ Group                 │   │
│  │  layer   │   │  layer                │   │
│  │  rect    │   │  rect / hint          │   │
│  │  render()│   │  layout_fn (vbox/hbox)│   │
│  │  data    │   │  ├─ Component         │   │
│  └──────────┘   │  └─ Group (nested)    │   │
│                 └───────────────────────┘   │
│                                             │
│  Event: ezt_on / ezt_off / ezt_emit         │
│  Focus: ezt_focus / ezt_focus_next/prev     │
│  Timer: ezt_timer_new / ezt_timer_free      │
└─────────────────────────────────────────────┘
```

### Component

The basic renderable unit. Has a `layer` (draw order), `rect` (absolute position), and a `render` callback.

```c
ezt_comp_t *c = ezt_comp_new(layer, render_fn, data);
c->rect       = EZT_RECT(x, y, w, h);
c->focusable  = true;   // included in focus cycling
```

### Group

A container that holds components and nested groups, with its own `layer`.

```c
ezt_group_t *g = ezt_group_new(layer);
g->rect        = EZT_RECT(0, 0, sw, sh);
g->layout_fn   = ezt_layout_vbox;   // automatic vertical layout

comp->hint = EZT_RECT(0, 0, 0, 3); // fixed 3 rows
flex->hint = EZT_RECT(0, 0, 0, 0); // flex: fill remaining space

ezt_group_add_comp(g, comp);
ezt_group_add_comp(g, flex);
ezt_group_layout(g);                // compute rects for all children
```

### Layer Rules

Within any container, items are rendered in ascending layer order (lower = behind, higher = in front). Items with the same layer are rendered in insertion order. Components and groups are compared by their own `layer` value.

### Events

Pure pub/sub, independent of the component tree.

```c
// subscribe
ezt_sub_t *sub = ezt_on(EZT_EV_KEY, handler, userdata);

// unsubscribe
ezt_off(sub);

// custom events
uint32_t MY_EV = ezt_event_register("my_event");
ezt_emit(MY_EV, &payload);
```

Built-in events: `EZT_EV_KEY`, `EZT_EV_MOUSE`, `EZT_EV_RESIZE`, `EZT_EV_TIMER`, `EZT_EV_FOCUS`, `EZT_EV_BLUR`.

### Focus

```c
comp->focusable = true;
ezt_focus(comp);          // set focus directly
ezt_focus_next();         // cycle forward (Tab)
ezt_focus_prev();         // cycle backward (Shift+Tab)
ezt_is_focused(comp);     // check in render callback to pick style
```

### Timer

```c
// fire every 500ms
ezt_timer_t *tmr = ezt_timer_new(500, true, NULL);
ezt_sub_t   *sub = ezt_on(EZT_EV_TIMER, on_timer, NULL);

// stop
ezt_timer_free(tmr);
ezt_off(sub);
```

---

## Examples

| Binary | Description |
|--------|-------------|
| `build/hello` | Minimal: centered box with Unicode text |
| `build/demo` | List + text input, Tab to switch focus |
| `build/tree_demo` | Full demo: comp/group/scene, nested layout, timer, event log |
| `build/imgtest [file] [--halfblock\|--quadrant]` | Image viewer: +/- to zoom, arrow/hjkl to pan |

```sh
./build/hello
./build/demo
./build/tree_demo
./build/imgtest pic.png
./build/imgtest pic.png --quadrant
```

---

## Draw API

```c
// Stateless (recommended)
ezt_draw_text(x, y, "Hello", style);
ezt_draw_fill(x, y, w, h, ' ', style);
ezt_draw_box (x, y, w, h, EZT_BOX_ROUNDED, style);
ezt_draw_hbar(x, y, w, style);
ezt_draw_vbar(x, y, h, style);

// Creating a style
EZT_STYLE(fg, bg, attr)
EZT_STYLE(EZT_WHITE, EZT_BLUE, EZT_ATTR_BOLD | EZT_ATTR_UNDERLINE)

// Colors
EZT_COLOR_DEFAULT
EZT_COLOR_16(n)       // n = 0..15
EZT_COLOR_256(n)      // n = 0..255
EZT_COLOR_RGB(r,g,b)  // 24-bit truecolor

// Screen
ezt_clear();
ezt_refresh();
```

Box border styles: `EZT_BOX_ASCII`, `EZT_BOX_SINGLE`, `EZT_BOX_DOUBLE`, `EZT_BOX_ROUNDED`, `EZT_BOX_BOLD`.

---

## Widgets

Lightweight stateful widgets usable inside any render callback.

```c
// Text input
char buf[128];
ezt_input_t inp;
ezt_input_init(&inp, buf, sizeof(buf));
// in key handler:
if (ezt_input_feed(&inp, kd)) { /* Enter pressed */ }
// in render callback:
ezt_input_draw(&inp, x, y, w, style);

// List selection
const char *items[] = { "Apple", "Banana", "Cherry" };
ezt_list_t lst;
ezt_list_init(&lst, items, 3);
// in key handler:
ezt_list_feed(&lst, kd);
// in render callback:
ezt_list_draw(&lst, x, y, w, h, normal_style, selected_style);
```

---

## Image Rendering

```c
// load with stb_image
uint8_t *rgba = stbi_load("image.png", &w, &h, &n, 4);

// render (rgba: 4 bytes per pixel)
ezt_image(x, y, rgba, w, h);

// switch to quadrant (higher resolution, universally supported)
ezt_img_set_backend(EZT_IMG_QUADRANT);

// query cell aspect ratio for correct image proportions
float aspect = ezt_cell_aspect(); // cell height / cell width
```

| Backend | Sub-pixels per cell | Notes |
|---------|--------------------:|-------|
| `EZT_IMG_HALFBLOCK` | 1×2 | Default, universal |
| `EZT_IMG_QUADRANT`  | 2×2 | Higher resolution, universal (U+2580–U+259F) |

---

## Project Structure

```
eztui/
├── include/
│   ├── eztui.h          # public API
│   └── stb_image.h      # image loading (single-header, public domain)
├── src/
│   ├── comp.c           # Component
│   ├── group.c          # Group + render / layout helpers
│   ├── scene.c          # Scene (global top-level container)
│   ├── layout.c         # vbox / hbox
│   ├── focus.c          # focus management
│   ├── event.c          # pub/sub event system
│   ├── timer.c          # timer
│   ├── input.c          # terminal input parsing
│   ├── screen.c         # cell buffer + draw API
│   ├── widget.c         # input / list widgets
│   ├── image.c          # image rendering backends
│   ├── color.c          # color conversion
│   ├── unicode.c        # UTF-8 / wcwidth
│   ├── term.c           # terminal escape sequences
│   └── eztui.c          # init / cleanup / tick
├── platform/
│   ├── posix.c          # POSIX termios / SIGWINCH / select
│   └── win32.c          # Windows (not yet implemented)
├── examples/
│   ├── hello.c
│   ├── demo.c
│   ├── tree_demo.c
│   └── imgtest.c
├── API.md               # full API reference
└── Makefile
```

---

## License

MIT
