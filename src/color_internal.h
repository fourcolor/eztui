#pragma once

#include <stdint.h>
#include "eztui.h"

typedef enum {
    EZT_COLORCAP_UNKNOWN = 0,
    EZT_COLORCAP_NONE,
    EZT_COLORCAP_16,
    EZT_COLORCAP_256,
    EZT_COLORCAP_TRUECOLOR,
} ezt_color_cap_t;

ezt_color_cap_t color_detect(void);
uint32_t        color_downgrade(uint32_t color, ezt_color_cap_t cap);
