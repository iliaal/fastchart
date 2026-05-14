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

  Low-level SVG element emitters operating on a smart_str. The
  fastchart_target_t SVG backend calls these for each primitive; the
  document open/close emit the <?xml ...><svg>...</svg> envelope.

  Coordinate precision is fixed at one decimal place (0.1px in SVG
  user units). Chart geometry never needs finer; smaller files for
  no visual cost.

  Attribute escaping covers the five XML predefined entities. Caller-
  supplied text (chart titles, labels) is escaped via fc_svg_escape.
*/

#ifndef FASTCHART_SVG_H
#define FASTCHART_SVG_H

#include "php.h"
#include "Zend/zend_smart_str.h"
#include <stdint.h>

/* Append a coordinate (or any 1-decimal float) to buf with trailing
 * zero and decimal-point trimmed where possible. "12.0" -> "12",
 * "12.50" -> "12.5". */
void fc_svg_fmt_num(smart_str *buf, double v);

/* Append a color attribute value. If alpha == 255, "#RRGGBB"; else
 * "rgba(R,G,B,A.AAA)" with A as 0..1 to three decimals. */
void fc_svg_fmt_color(smart_str *buf, uint32_t rgba);

/* Escape `s` (len bytes; may be NULL when len==0) for use inside an
 * XML attribute or text node and append to buf. Replaces & < > " '
 * with the standard entities. */
void fc_svg_escape(smart_str *buf, const char *s, size_t len);

/* Document open: <?xml ...><svg ...>. Caller follows with primitives,
 * then fc_svg_emit_doc_close. */
void fc_svg_emit_doc_open(smart_str *buf, int width, int height);
void fc_svg_emit_doc_close(smart_str *buf);

/* Group wrappers. `klass` may be NULL. */
void fc_svg_emit_g_open(smart_str *buf, const char *klass);
void fc_svg_emit_g_close(smart_str *buf);

/* Primitives. rgba == 0xAARRGGBB. thickness ignored when 0 or when
 * fill && !stroke. dash is the FASTCHART_DASH_* value from
 * fastchart_target.h. */

void fc_svg_emit_line(smart_str *buf,
                      double x0, double y0, double x1, double y1,
                      uint32_t rgba, int thickness, int dash);

void fc_svg_emit_rect(smart_str *buf,
                      double x, double y, double w, double h,
                      uint32_t rgba, int fill, int thickness);

void fc_svg_emit_polygon(smart_str *buf,
                          const int *xs, const int *ys, int n,
                          uint32_t rgba, int fill, int thickness);

void fc_svg_emit_ellipse(smart_str *buf,
                          double cx, double cy, double rx, double ry,
                          uint32_t rgba, int fill, int thickness);

/* Pie wedge / open arc. start_deg, end_deg in degrees, 0° east,
 * increasing clockwise (matches libgd's gdImageFilledArc). If
 * `fill` is non-zero, emits a closed wedge from (cx,cy); else an
 * open arc stroke from the start point to the end point. */
void fc_svg_emit_path_arc(smart_str *buf,
                           double cx, double cy, double rx, double ry,
                           double start_deg, double end_deg,
                           uint32_t rgba, int fill, int thickness);

/* (x, y) is the baseline. `family` is the CSS font-family (already
 * resolved via FreeType). size_px is in user units. */
void fc_svg_emit_text(smart_str *buf,
                       double x, double y,
                       const char *family, double size_px,
                       uint32_t rgba, double angle_deg, int align,
                       const char *text, size_t text_len);

/* Open a clip-path scope. Caller picks a unique `id`. After this
 * call subsequent primitives are clipped to (x,y,w,h) until the
 * matching close. */
void fc_svg_emit_clip_open(smart_str *buf, int id,
                            double x, double y, double w, double h);
void fc_svg_emit_clip_close(smart_str *buf);

#endif /* FASTCHART_SVG_H */
