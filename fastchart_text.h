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

#include <gd.h>

typedef enum {
    FASTCHART_ALIGN_LEFT   = 0,
    FASTCHART_ALIGN_CENTER = 1,
    FASTCHART_ALIGN_RIGHT  = 2,
} fastchart_align;

/* Draw `text` at (x, y) with the given alignment. y is the baseline.
 * `text` must be NUL-terminated UTF-8.
 * Returns 0 on success, -1 if libgd refused the font (sets *err).
 * `err_buf` (buf_n bytes) receives a libgd error string when present. */
int fastchart_text_draw(gdImagePtr im,
                        const char *font_path, double font_size,
                        int color, int x, int y,
                        fastchart_align align,
                        const char *text,
                        char *err_buf, size_t err_buf_n);

/* Same as fastchart_text_draw but rotates the text counter-clockwise
 * by `angle_deg` (typical: 0, 45, 90). The anchor (x, y) is the
 * alignment point of the unrotated bounding box; libgd rotates
 * around that anchor. */
int fastchart_text_draw_rotated(gdImagePtr im,
                                const char *font_path, double font_size,
                                int color, int x, int y,
                                fastchart_align align, double angle_deg,
                                const char *text,
                                char *err_buf, size_t err_buf_n);

/* Measure rendered bounds. *out_w and *out_h are populated on success.
 * Returns 0 on success, -1 on failure. */
int fastchart_text_measure(const char *font_path, double font_size,
                           const char *text,
                           int *out_w, int *out_h,
                           char *err_buf, size_t err_buf_n);

#endif /* FASTCHART_TEXT_H */
