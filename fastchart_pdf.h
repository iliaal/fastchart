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

  PDF render backend. Compiled only when configured --with-pdfio; the
  whole translation unit and every call site is guarded by
  HAVE_FASTCHART_PDF. Vector output: chart bodies emit the same
  fastchart_target_* primitives as the SVG path, and target.c routes
  them here when t->kind == FASTCHART_TARGET_PDF. We translate to PDF
  content-stream operators via the system pdfio library (msweet.org).

  Coordinate systems differ: fastchart/SVG use a top-left origin with
  +y pointing down; PDF uses a bottom-left origin with +y up. Every
  emitter flips y as (page_height - y). Glyph outlines come from the
  shared glyph cache as TrueType quadratics ('Q'); PDF has only cubic
  beziers, so each quadratic is degree-elevated at emit time.
*/

#ifndef FASTCHART_PDF_H
#define FASTCHART_PDF_H

#ifdef HAVE_FASTCHART_PDF

#include "php.h"
#include "Zend/zend_smart_str.h"
#include <stdint.h>

/* Opaque PDF document/page state. Defined in fastchart_pdf.c so pdfio.h
 * stays out of the widely-included fastchart_target.h. target.c holds it
 * as a void* and passes it back to these emitters. */
typedef struct fc_pdf_state fc_pdf_state;

/* Open a single-page PDF sized width x height (logical px == PDF points
 * at the 96-DPI baseline). All bytes are streamed into `out` (a caller-
 * owned smart_str) as pdfio flushes. Returns NULL on failure. */
fc_pdf_state *fc_pdf_doc_open(smart_str *out, int width, int height);

/* Close the page content stream and finalize the document (xref +
 * trailer flush through the output callback), then free the state.
 * Returns 0 on success, -1 if pdfio reported a write error. */
int fc_pdf_doc_close(fc_pdf_state *st);

/* Bailout-unwind teardown: sets the aborted flag (output callback drops
 * the close-time flush instead of appending to request memory) and
 * closes the document, releasing all malloc'd pdfio state. */
void fc_pdf_doc_abort(fc_pdf_state *st);

/* Primitives. Colors are packed 0xAARRGGBB: alpha byte 255 is opaque,
 * a value in between is flattened against the captured page background
 * (see the alpha note in fastchart_pdf.c), and alpha byte 0 is a
 * suppressed shape (paints nothing). thickness >= 1; fill is 0/1; dash
 * is FASTCHART_DASH_*. Polygon takes parallel xs/ys int arrays (target.c
 * already splits fastchart_point_t). */
void fc_pdf_emit_line(fc_pdf_state *st, double x0, double y0,
                       double x1, double y1, uint32_t rgba,
                       int thickness, int dash);
void fc_pdf_emit_rect(fc_pdf_state *st, double x, double y,
                       double w, double h, uint32_t rgba,
                       int fill, int thickness);
void fc_pdf_emit_polygon(fc_pdf_state *st, const int *xs, const int *ys,
                          int n, uint32_t rgba, int fill, int thickness);
void fc_pdf_emit_polyline(fc_pdf_state *st, const int *xs, const int *ys,
                           int n, uint32_t rgba, int thickness);
void fc_pdf_emit_ellipse(fc_pdf_state *st, double cx, double cy,
                          double rx, double ry, uint32_t rgba,
                          int fill, int thickness);
void fc_pdf_emit_arc(fc_pdf_state *st, double cx, double cy,
                      double rx, double ry,
                      double start_deg, double end_deg,
                      uint32_t rgba, int fill, int thickness);
void fc_pdf_emit_text_as_path(fc_pdf_state *st, double x, double y,
                               const char *font_path, double size_px,
                               uint32_t rgba, double angle_deg, int align,
                               const char *text, size_t text_len);
void fc_pdf_emit_clip_open(fc_pdf_state *st, double x, double y,
                            double w, double h);
void fc_pdf_emit_clip_close(fc_pdf_state *st);

/* Gradient fills. v1 has no PDF axial/radial shading; pdfio exposes no
 * shading helper. These fall back to a solid fill of from_rgb so charts
 * stay correct (gradient is cosmetic). dir/to_rgb are accepted for
 * signature parity with the SVG backend. */
void fc_pdf_emit_gradient_rect(fc_pdf_state *st, double x, double y,
                                double w, double h, uint32_t from_rgb,
                                uint32_t to_rgb, int dir);
void fc_pdf_emit_gradient_polygon(fc_pdf_state *st, const int *xs,
                                   const int *ys, int n, uint32_t from_rgb,
                                   uint32_t to_rgb, int dir);

#endif /* HAVE_FASTCHART_PDF */
#endif /* FASTCHART_PDF_H */
