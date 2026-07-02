/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2025-2026, Ilia Alshanetsky                            |
  | Copyright (c) 2025-2026, Advanced Internet Designs Inc.              |
  +----------------------------------------------------------------------+
  | This source file is subject to the BSD 3-Clause license that is      |
  | bundled with this package in the file LICENSE.                       |
  +----------------------------------------------------------------------+
  | Author: Ilia Alshanetsky <ilia@ilia.ws>                              |
  +----------------------------------------------------------------------+
*/

#ifndef FASTCHART_TEXT_H
#define FASTCHART_TEXT_H

#include "fastchart_target.h"

typedef enum {
    FASTCHART_ALIGN_LEFT   = 0,
    FASTCHART_ALIGN_CENTER = 1,
    FASTCHART_ALIGN_RIGHT  = 2,
} fastchart_align;

/* Draw `text` at (x, y) with the given alignment. y is the baseline.
 * `text` must be NUL-terminated UTF-8. `color` is a target color
 * handle (allocate via fastchart_target_color* on the same target).
 * Returns 0 on success, -1 if the font cannot be resolved or used.
 * `err_buf` (buf_n bytes) receives a short error string when present. */
int fastchart_text_draw(fastchart_target_t *t,
                        const char *font_path, double font_size,
                        int color, int x, int y,
                        fastchart_align align,
                        const char *text,
                        char *err_buf, size_t err_buf_n);

/* Same as fastchart_text_draw but rotates the text counter-clockwise
 * by `angle_deg` (typical: 0, 45, 90). The anchor (x, y) is the
 * alignment point of the unrotated bounding box. */
int fastchart_text_draw_rotated(fastchart_target_t *t,
                                const char *font_path, double font_size,
                                int color, int x, int y,
                                fastchart_align align, double angle_deg,
                                const char *text,
                                char *err_buf, size_t err_buf_n);

/* Measure rendered bounds. *out_w and *out_h are populated on success.
 * `t` carries the DPI used during the measurement so the bounds match
 * what the corresponding draw call will produce. `t` may be NULL for a
 * pure-measure context with no canvas. Returns 0 on success, -1 on
 * failure. */
int fastchart_text_measure(fastchart_target_t *t,
                           const char *font_path, double font_size,
                           const char *text,
                           int *out_w, int *out_h,
                           char *err_buf, size_t err_buf_n);

/* Shared UTF-8 next-codepoint walker for every text consumer
 * (measurement, SVG glyph emitter, PDF glyph emitter). One definition
 * so the copies cannot drift: an earlier inline copy in the measurer
 * SKIPPED invalid bytes (0 width) while the emitters substituted
 * U+FFFD with a real advance — layout under-reserved on exactly the
 * malformed input the walkers exist to survive. Truncated / invalid
 * sequences yield U+FFFD and advance one byte; returns NULL at end. */
static zend_always_inline const unsigned char *fc_utf8_next_cp(
    const unsigned char *p, const unsigned char *end, uint32_t *out_cp)
{
	if (p >= end) return NULL;
	if (*p < 0x80) { *out_cp = *p; return p + 1; }
	if ((*p & 0xE0) == 0xC0 && p + 1 < end) {
		*out_cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
		return p + 2;
	}
	if ((*p & 0xF0) == 0xE0 && p + 2 < end) {
		*out_cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
		return p + 3;
	}
	if ((*p & 0xF8) == 0xF0 && p + 3 < end) {
		*out_cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12)
		       | ((p[2] & 0x3F) << 6)  |  (p[3] & 0x3F);
		return p + 4;
	}
	*out_cp = 0xFFFD;
	return p + 1;
}

#endif /* FASTCHART_TEXT_H */
