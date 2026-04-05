#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Decode one UTF-8 codepoint from *p, advance *p past it.
 * Returns 0 at NUL, 0xFFFD on invalid sequence. */
uint32_t utf8_decode(const char **p);

/* Encode codepoint cp into buf (up to 4 bytes, no NUL). Returns byte count. */
int utf8_encode(uint32_t cp, char buf[4]);

/* Display cell width of a single codepoint (0, 1, or 2). */
int ucp_width(uint32_t cp);

/* Display cell width of a NUL-terminated UTF-8 string. */
int utf8_width(const char *s);

/* Display cell width of the first n bytes of s. */
int utf8_width_n(const char *s, int n);
