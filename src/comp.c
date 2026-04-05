/* comp.c — component: a renderable item with a layer */

#include <stdlib.h>
#include "eztui.h"

ezt_comp_t *ezt_comp_new(int layer,
                          void (*render)(ezt_comp_t *, ezt_rect_t),
                          void *data) {
    ezt_comp_t *c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->layer  = layer;
    c->render = render;
    c->data   = data;
    c->style  = EZT_STYLE_DEFAULT;
    return c;
}

void ezt_comp_free(ezt_comp_t *comp) {
    if (!comp) return;
    if (comp->destroy) comp->destroy(comp);
    free(comp);
}
