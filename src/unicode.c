/* unicode.c — UTF-8 decode and character display-width (wcwidth) */

#include <stdint.h>
#include "unicode_internal.h"

/* -------------------------------------------------------------------------
 * UTF-8 decode
 * Returns codepoint and advances *p.  On error returns 0xFFFD.
 * ------------------------------------------------------------------------- */
uint32_t utf8_decode(const char **p) {
    const unsigned char *s = (const unsigned char *)*p;
    uint32_t cp;
    int extra;

    if (*s == 0) return 0;

    if (*s < 0x80) {
        cp = *s++;
        extra = 0;
    } else if ((*s & 0xE0) == 0xC0) {
        cp = *s++ & 0x1F;
        extra = 1;
    } else if ((*s & 0xF0) == 0xE0) {
        cp = *s++ & 0x0F;
        extra = 2;
    } else if ((*s & 0xF8) == 0xF0) {
        cp = *s++ & 0x07;
        extra = 3;
    } else {
        (*p)++;
        return 0xFFFD;
    }

    for (int i = 0; i < extra; i++) {
        if ((*s & 0xC0) != 0x80) {
            *p = (const char *)s;
            return 0xFFFD;
        }
        cp = (cp << 6) | (*s++ & 0x3F);
    }
    *p = (const char *)s;
    return cp;
}

/* -------------------------------------------------------------------------
 * UTF-8 encode
 * Writes up to 4 bytes into buf (no NUL terminator), returns byte count.
 * ------------------------------------------------------------------------- */
int utf8_encode(uint32_t cp, char buf[4]) {
    if (cp < 0x80) {
        buf[0] = (char)cp;
        return 1;
    } else if (cp < 0x800) {
        buf[0] = (char)(0xC0 | (cp >> 6));
        buf[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp < 0x10000) {
        buf[0] = (char)(0xE0 | (cp >> 12));
        buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    } else {
        buf[0] = (char)(0xF0 | (cp >> 18));
        buf[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        buf[2] = (char)(0x80 | ((cp >> 6)  & 0x3F));
        buf[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
}

/* -------------------------------------------------------------------------
 * Display width (wcwidth subset)
 *
 * Covers:
 *   - C0/C1 controls → 0
 *   - Combining / zero-width categories → 0
 *   - CJK wide blocks → 2
 *   - Everything else → 1
 *
 * Based on Unicode 15 East Asian Width data (condensed range table).
 * ------------------------------------------------------------------------- */

/* Zero-width ranges: sorted, non-overlapping */
static const uint32_t s_zero[][2] = {
    {0x0000, 0x001F}, {0x007F, 0x009F}, /* C0/C1 controls */
    /* Combining diacritical marks */
    {0x0300, 0x036F},
    {0x0483, 0x0489},
    {0x0591, 0x05BD}, {0x05BF, 0x05BF}, {0x05C1, 0x05C2},
    {0x05C4, 0x05C5}, {0x05C7, 0x05C7},
    {0x0610, 0x061A}, {0x064B, 0x065F}, {0x0670, 0x0670},
    {0x06D6, 0x06DC}, {0x06DF, 0x06E4}, {0x06E7, 0x06E8},
    {0x06EA, 0x06ED},
    {0x0711, 0x0711}, {0x0730, 0x074A},
    {0x07A6, 0x07B0}, {0x07EB, 0x07F3},
    {0x0816, 0x0823}, {0x0825, 0x082D}, {0x0829, 0x082D},
    {0x0900, 0x0902}, {0x093A, 0x093A}, {0x093C, 0x093C},
    {0x0941, 0x0948}, {0x094D, 0x094D},
    {0x0951, 0x0957}, {0x0962, 0x0963},
    /* Hangul Jamo combining */
    {0x1160, 0x11FF},
    /* General combining */
    {0x20D0, 0x20F0},
    {0xFE20, 0xFE2F},
    /* Zero-width no-break space / BOM */
    {0xFEFF, 0xFEFF},
    /* Tags block */
    {0xE0000, 0xE01FF},
};

/* Wide (double-width) ranges */
static const uint32_t s_wide[][2] = {
    {0x1100, 0x115F},   /* Hangul Jamo */
    {0x2E80, 0x2EFF},   /* CJK Radicals Supplement */
    {0x2F00, 0x2FDF},   /* Kangxi Radicals */
    {0x2FF0, 0x2FFF},
    {0x3000, 0x303F},   /* CJK Symbols and Punctuation */
    {0x3040, 0x33FF},   /* Hiragana..CJK Compatibility */
    {0x3400, 0x4DBF},   /* CJK Extension A */
    {0x4E00, 0x9FFF},   /* CJK Unified Ideographs */
    {0xA000, 0xA4CF},   /* Yi */
    {0xA960, 0xA97F},   /* Hangul Jamo Extended-A */
    {0xAC00, 0xD7AF},   /* Hangul Syllables */
    {0xD7B0, 0xD7FF},   /* Hangul Jamo Extended-B */
    {0xF900, 0xFAFF},   /* CJK Compatibility Ideographs */
    {0xFE10, 0xFE1F},   /* Vertical forms */
    {0xFE30, 0xFE4F},   /* CJK Compatibility Forms */
    {0xFF00, 0xFF60},   /* Fullwidth Forms */
    {0xFFE0, 0xFFE6},
    {0x1B000, 0x1B0FF}, /* Kana Supplement */
    {0x1F004, 0x1F004},
    {0x1F0CF, 0x1F0CF},
    {0x1F200, 0x1F2FF},
    {0x1F300, 0x1F64F}, /* Misc Symbols, Emoticons */
    {0x1F900, 0x1F9FF},
    {0x20000, 0x2FFFD}, /* CJK Extension B..F */
    {0x30000, 0x3FFFD}, /* CJK Extension G.. */
};

static bool in_ranges(const uint32_t ranges[][2], int count, uint32_t cp) {
    int lo = 0, hi = count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (cp < ranges[mid][0])      hi = mid - 1;
        else if (cp > ranges[mid][1]) lo = mid + 1;
        else                          return true;
    }
    return false;
}

int ucp_width(uint32_t cp) {
    if (cp == 0) return 0;
    int nz = (int)(sizeof(s_zero) / sizeof(s_zero[0]));
    if (in_ranges(s_zero, nz, cp)) return 0;
    int nw = (int)(sizeof(s_wide) / sizeof(s_wide[0]));
    if (in_ranges(s_wide, nw, cp)) return 2;
    return 1;
}

/* Display width of a NUL-terminated UTF-8 string */
int utf8_width(const char *s) {
    int w = 0;
    uint32_t cp;
    while ((cp = utf8_decode(&s)) != 0)
        w += ucp_width(cp);
    return w;
}

/* Display width of the first n bytes of s */
int utf8_width_n(const char *s, int n) {
    int w = 0;
    const char *end = s + n;
    while (s < end) {
        uint32_t cp = utf8_decode(&s);
        w += ucp_width(cp);
    }
    return w;
}
