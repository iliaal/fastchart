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

  PDF render backend — see fastchart_pdf.h for the design notes. Built
  only when configured --with-pdfio.
*/

#include "php.h"

#ifdef HAVE_FASTCHART_PDF

#include <math.h>
#include <pdfio.h>
#include <pdfio-content.h>

#include "fastchart_target.h"
#include "php_fastchart.h"
#include "fastchart_pdf.h"
#include "fastchart_text.h"

#include <ft2build.h>
#include FT_FREETYPE_H

struct fc_pdf_state {
	pdfio_file_t   *pdf;
	pdfio_stream_t *st;
	smart_str      *out;   /* borrowed; output callback appends here */
	double          h;     /* page height, for the y-down -> y-up flip */
	double          pw;    /* page width, for the canvas-bg capture below */
	int             err;   /* sticky: set by the pdfio error callback */
	int             aborted; /* 1 = drop output bytes (bailout unwind) */
	/* This pdfio build exposes no transparency / ExtGState API (no
	 * /ca|/CA), so per-element alpha is flattened against the page
	 * background captured from the chart's first full-canvas fill. Real
	 * layered transparency isn't representable; single-layer translucent
	 * fills over the background composite correctly. */
	uint32_t        bg;    /* 0xAARRGGBB; default opaque white */
	int             bg_set;
};

/* pdfio streams document bytes through this callback as it writes.
 * When the state is aborted (memory_limit bailout unwind), bytes are
 * swallowed instead of appended: smart_str_appendl could re-enter the
 * bailout from inside the cleanup path. */
static ssize_t fc_pdf_output_cb(void *ctx, const void *data, size_t len)
{
	fc_pdf_state *s = (fc_pdf_state *)ctx;
	if (!s->aborted) {
		smart_str_appendl(s->out, (const char *)data, len);
	}
	return (ssize_t)len;
}

static bool fc_pdf_error_cb(pdfio_file_t *pdf, const char *message, void *data)
{
	(void)pdf;
	/* pdfio invokes this for warnings as well as errors; its contract is
	 * to return true (continue) for non-fatal "WARNING:"-prefixed
	 * messages and false only for real errors. Treating a recoverable
	 * warning as fatal aborts the content stream and fails an otherwise
	 * valid render. */
	int is_warning = message && !strncmp(message, "WARNING:", 8);
	fc_pdf_state *st = (fc_pdf_state *)data;
	if (!is_warning && st) st->err = 1;
	return is_warning;
}

/* y-down (fastchart) -> y-up (PDF). */
static inline double FY(const fc_pdf_state *s, double y) { return s->h - y; }

static inline double cc(uint32_t v) { return (double)v / 255.0; }

/* A fully-transparent handle (alpha byte 0) means "paint nothing": the
 * SVG backend emits rgba(...,0), which renders invisibly, so the PDF
 * backend must skip the paint to match. Legacy bare RGB (also alpha
 * byte 0, but meaning opaque) reaches the backend only through the
 * gradient fallbacks — which force 0xFF — and the canvas-background
 * capture in fc_pdf_emit_rect, both handled explicitly. Everywhere else
 * alpha 0 is a suppressed shape, e.g. a drop shadow at setShadowAlpha(127)
 * (fastchart_effects.c maps 127 to alpha byte 0). */
static inline int fc_pdf_transparent(uint32_t rgba)
{
	return ((rgba >> 24) & 0xFFu) == 0u;
}

/* Flatten a translucent 0xAARRGGBB onto the page background. Alpha byte
 * 255 (opaque) and 0 (legacy bare-RGB canvas fill, also opaque) pass
 * through; genuinely-transparent handles are caught by fc_pdf_transparent
 * at the emitters before reaching here. */
static uint32_t fc_pdf_flatten(const fc_pdf_state *s, uint32_t rgba)
{
	uint32_t a = (rgba >> 24) & 0xFF;
	if (a == 0u || a == 0xFFu) return rgba;
	double af = (double)a / 255.0;
	double br = (double)((s->bg >> 16) & 0xFF);
	double bg = (double)((s->bg >>  8) & 0xFF);
	double bb = (double)( s->bg        & 0xFF);
	double fr = (double)((rgba >> 16) & 0xFF);
	double fg = (double)((rgba >>  8) & 0xFF);
	double fb = (double)( rgba        & 0xFF);
	uint32_t r = (uint32_t)(fr * af + br * (1.0 - af) + 0.5);
	uint32_t g = (uint32_t)(fg * af + bg * (1.0 - af) + 0.5);
	uint32_t b = (uint32_t)(fb * af + bb * (1.0 - af) + 0.5);
	return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static void set_fill(fc_pdf_state *s, uint32_t rgba)
{
	rgba = fc_pdf_flatten(s, rgba);
	pdfioContentSetFillColorDeviceRGB(s->st,
	    cc((rgba >> 16) & 0xFF), cc((rgba >> 8) & 0xFF), cc(rgba & 0xFF));
}

static void set_stroke(fc_pdf_state *s, uint32_t rgba, int thickness, int dash)
{
	rgba = fc_pdf_flatten(s, rgba);
	pdfioContentSetStrokeColorDeviceRGB(s->st,
	    cc((rgba >> 16) & 0xFF), cc((rgba >> 8) & 0xFF), cc(rgba & 0xFF));
	pdfioContentSetLineWidth(s->st, thickness > 0 ? (double)thickness : 1.0);
	double u = thickness > 0 ? (double)thickness : 1.0;
	if (dash == FASTCHART_DASH_DASHED)
		pdfioContentSetDashPattern(s->st, 0.0, 6.0 * u, 4.0 * u);
	else if (dash == FASTCHART_DASH_DOTTED)
		pdfioContentSetDashPattern(s->st, 0.0, 1.0 * u, 3.0 * u);
}

/* ============================================================ *
 * Lifecycle                                                     *
 * ============================================================ */

fc_pdf_state *fc_pdf_doc_open(smart_str *out, int width, int height)
{
	if (width <= 0 || height <= 0) return NULL;

	fc_pdf_state *s = ecalloc(1, sizeof(*s));
	s->out = out;
	s->h   = (double)height;
	s->pw  = (double)width;
	s->bg  = 0xFFFFFFFFu;  /* opaque white until the canvas fill is seen */

	pdfio_rect_t media = { 0.0, 0.0, (double)width, (double)height };
	/* Pass media as the crop box too: with a NULL crop box pdfio stamps
	 * a default A4/Letter CropBox, which clips our page in viewers that
	 * honor CropBox over MediaBox (Acrobat, Chrome, print pipelines). */
	s->pdf = pdfioFileCreateOutput(fc_pdf_output_cb, s, NULL, &media, &media,
	                                fc_pdf_error_cb, s);
	if (!s->pdf) { efree(s); return NULL; }

	pdfio_dict_t *dict = pdfioDictCreate(s->pdf);
	s->st = pdfioFileCreatePage(s->pdf, dict);
	if (!s->st) { pdfioFileClose(s->pdf); efree(s); return NULL; }

	return s;
}

int fc_pdf_doc_close(fc_pdf_state *s)
{
	if (!s) return -1;
	if (s->st)  pdfioStreamClose(s->st);
	int err = s->err;
	if (s->pdf) { if (!pdfioFileClose(s->pdf)) err = 1; }
	efree(s);
	return err ? -1 : 0;
}

/* Bailout-unwind teardown: releases every malloc'd pdfio object without
 * touching request memory beyond the state itself — the output callback
 * swallows the close-time flush. */
void fc_pdf_doc_abort(fc_pdf_state *s)
{
	if (!s) return;
	s->aborted = 1;
	(void)fc_pdf_doc_close(s);
}

/* ============================================================ *
 * Primitives                                                    *
 * ============================================================ */

void fc_pdf_emit_line(fc_pdf_state *s, double x0, double y0,
                       double x1, double y1, uint32_t rgba,
                       int thickness, int dash)
{
	if (fc_pdf_transparent(rgba)) return;
	pdfioContentSave(s->st);
	set_stroke(s, rgba, thickness, dash);
	pdfioContentPathMoveTo(s->st, x0, FY(s, y0));
	pdfioContentPathLineTo(s->st, x1, FY(s, y1));
	pdfioContentStroke(s->st);
	pdfioContentRestore(s->st);
}

void fc_pdf_emit_rect(fc_pdf_state *s, double x, double y,
                       double w, double h, uint32_t rgba,
                       int fill, int thickness)
{
	uint32_t a = (rgba >> 24) & 0xFF;
	int is_canvas = 0;
	if (fill && !s->bg_set && (a == 0xFFu || a == 0u) &&
	    x <= 0.5 && y <= 0.5 && w >= s->pw - 1.0 && h >= s->h - 1.0) {
		/* Capture the chart's opaque full-canvas fill as the background
		 * that later translucent fills composite against. */
		s->bg = 0xFF000000u | (rgba & 0x00FFFFFFu);
		s->bg_set = 1;
		is_canvas = 1;
	}
	/* Suppressed shape: a fully-transparent fill or stroke paints
	 * nothing. Skip before emitting any path so the content stream is
	 * byte-identical to omitting the shape. The canvas-bg capture above
	 * is the sole legacy bare-RGB exception and always draws. */
	if (a == 0u && !is_canvas) return;

	pdfioContentSave(s->st);
	/* PDF rect origin is the lower-left corner; the fastchart rect is
	 * given by its upper-left (x,y), so the flipped lower-left y is
	 * FY(y+h). */
	pdfioContentPathRect(s->st, x, FY(s, y + h), w, h);
	if (fill) {
		set_fill(s, rgba);
		pdfioContentFill(s->st, false);
	} else {
		set_stroke(s, rgba, thickness, FASTCHART_DASH_SOLID);
		pdfioContentStroke(s->st);
	}
	pdfioContentRestore(s->st);
}

void fc_pdf_emit_polygon(fc_pdf_state *s, const int *xs, const int *ys,
                          int n, uint32_t rgba, int fill, int thickness)
{
	if (n < 2) return;
	if (fc_pdf_transparent(rgba)) return;
	pdfioContentSave(s->st);
	pdfioContentPathMoveTo(s->st, (double)xs[0], FY(s, (double)ys[0]));
	for (int i = 1; i < n; i++)
		pdfioContentPathLineTo(s->st, (double)xs[i], FY(s, (double)ys[i]));
	pdfioContentPathClose(s->st);
	if (fill) {
		set_fill(s, rgba);
		pdfioContentFill(s->st, false);
	} else {
		set_stroke(s, rgba, thickness, FASTCHART_DASH_SOLID);
		pdfioContentStroke(s->st);
	}
	pdfioContentRestore(s->st);
}

/* Open polyline: one path, stroked once. The SVG backend emits a single
 * <polyline> with joins; matching that here (rather than n-1 independent
 * line segments) avoids butt-cap notches at interior vertices for
 * thickness > 1. */
void fc_pdf_emit_polyline(fc_pdf_state *s, const int *xs, const int *ys,
                           int n, uint32_t rgba, int thickness, int dash)
{
	if (n < 2) return;
	if (fc_pdf_transparent(rgba)) return;
	pdfioContentSave(s->st);
	set_stroke(s, rgba, thickness, dash);
	pdfioContentPathMoveTo(s->st, (double)xs[0], FY(s, (double)ys[0]));
	for (int i = 1; i < n; i++)
		pdfioContentPathLineTo(s->st, (double)xs[i], FY(s, (double)ys[i]));
	pdfioContentStroke(s->st);
	pdfioContentRestore(s->st);
}

/* Quarter-arc bezier constant: control-point offset for a 90° arc. */
#define FC_PDF_KAPPA 0.5522847498307936

void fc_pdf_emit_ellipse(fc_pdf_state *s, double cx, double cy,
                          double rx, double ry, uint32_t rgba,
                          int fill, int thickness)
{
	if (fc_pdf_transparent(rgba)) return;
	double ox = FC_PDF_KAPPA * rx, oy = FC_PDF_KAPPA * ry;
	double Y = FY(s, cy);          /* center in PDF space */
	/* Four cubic quadrants, starting at the +x point. Sign of oy is in
	 * PDF space; the y-flip only moves the center, the symmetric offsets
	 * are unaffected. */
	pdfioContentSave(s->st);
	pdfioContentPathMoveTo(s->st, cx + rx, Y);
	pdfioContentPathCurve(s->st, cx + rx, Y + oy, cx + ox, Y + ry, cx, Y + ry);
	pdfioContentPathCurve(s->st, cx - ox, Y + ry, cx - rx, Y + oy, cx - rx, Y);
	pdfioContentPathCurve(s->st, cx - rx, Y - oy, cx - ox, Y - ry, cx, Y - ry);
	pdfioContentPathCurve(s->st, cx + ox, Y - ry, cx + rx, Y - oy, cx + rx, Y);
	pdfioContentPathClose(s->st);
	if (fill) {
		set_fill(s, rgba);
		pdfioContentFill(s->st, false);
	} else {
		set_stroke(s, rgba, thickness, FASTCHART_DASH_SOLID);
		pdfioContentStroke(s->st);
	}
	pdfioContentRestore(s->st);
}

/* Append a single ≤90° arc segment as one cubic bezier to the current
 * path. Angles in radians, fastchart convention (0=east, CW with +y
 * south); we flip y via FY at point-emit. Assumes the current point is
 * already at the segment start (caller did MoveTo/LineTo). */
static void fc_pdf_arc_segment(fc_pdf_state *s, double cx, double cy,
                                double rx, double ry, double a0, double a1)
{
	double da = a1 - a0;
	double k = (4.0 / 3.0) * tan(da / 4.0);
	double x0 = cx + rx * cos(a0), y0 = cy + ry * sin(a0);
	double x1 = cx + rx * cos(a1), y1 = cy + ry * sin(a1);
	/* Tangent-derived control points (derivative of the parametric arc). */
	double c1x = x0 - k * rx * sin(a0), c1y = y0 + k * ry * cos(a0);
	double c2x = x1 + k * rx * sin(a1), c2y = y1 - k * ry * cos(a1);
	pdfioContentPathCurve(s->st, c1x, FY(s, c1y), c2x, FY(s, c2y),
	                      x1, FY(s, y1));
}

void fc_pdf_emit_arc(fc_pdf_state *s, double cx, double cy,
                      double rx, double ry,
                      double start_deg, double end_deg,
                      uint32_t rgba, int fill, int thickness)
{
	if (fc_pdf_transparent(rgba)) return;
	double sweep = end_deg - start_deg;
	if (sweep < 0) sweep += 360.0;
	if (sweep >= 359.999) {
		fc_pdf_emit_ellipse(s, cx, cy, rx, ry, rgba, fill, thickness);
		return;
	}

	double a0 = start_deg * M_PI / 180.0;
	double total = sweep * M_PI / 180.0;
	int segs = (int)ceil(sweep / 90.0);
	if (segs < 1) segs = 1;
	double seg = total / segs;

	double sx = cx + rx * cos(a0), sy = cy + ry * sin(a0);

	pdfioContentSave(s->st);
	if (fill) {
		/* Wedge: center -> arc start -> arc -> close. */
		pdfioContentPathMoveTo(s->st, cx, FY(s, cy));
		pdfioContentPathLineTo(s->st, sx, FY(s, sy));
	} else {
		pdfioContentPathMoveTo(s->st, sx, FY(s, sy));
	}
	double a = a0;
	for (int i = 0; i < segs; i++) {
		fc_pdf_arc_segment(s, cx, cy, rx, ry, a, a + seg);
		a += seg;
	}
	if (fill) {
		pdfioContentPathClose(s->st);
		set_fill(s, rgba);
		pdfioContentFill(s->st, false);
	} else {
		set_stroke(s, rgba, thickness, FASTCHART_DASH_SOLID);
		pdfioContentStroke(s->st);
	}
	pdfioContentRestore(s->st);
}

/* ---- text as path -------------------------------------------------- */

/* UTF-8 walking goes through fc_utf8_next_cp (fastchart_text.h). */
#define fc_pdf_utf8_next fc_utf8_next_cp

/* Replay one cached glyph into the current path. Glyph pts are y-down
 * pixel coords local to the baseline origin; we work in a local frame
 * already translated to the anchor (and possibly rotated), so a glyph
 * point (gx,gy) maps to PDF-local (pen_x+gx, -gy). Quadratics ('Q') are
 * degree-elevated to cubics. Tracks current point for the elevation. */
static void fc_pdf_replay_glyph(fc_pdf_state *s,
                                 const fc_glyph_cache_entry *g, double pen_x,
                                 double *curx, double *cury)
{
	int pi = 0;
	for (int oi = 0; oi < g->n_ops; oi++) {
		char op = g->ops[oi];
		if (op == 'M') {
			double x = pen_x + g->pts[pi], y = -g->pts[pi + 1];
			pdfioContentPathMoveTo(s->st, x, y);
			*curx = x; *cury = y; pi += 2;
		} else if (op == 'L') {
			double x = pen_x + g->pts[pi], y = -g->pts[pi + 1];
			pdfioContentPathLineTo(s->st, x, y);
			*curx = x; *cury = y; pi += 2;
		} else if (op == 'Q') {
			double qx = pen_x + g->pts[pi],     qy = -g->pts[pi + 1];
			double ex = pen_x + g->pts[pi + 2], ey = -g->pts[pi + 3];
			double c1x = *curx + 2.0 / 3.0 * (qx - *curx);
			double c1y = *cury + 2.0 / 3.0 * (qy - *cury);
			double c2x = ex + 2.0 / 3.0 * (qx - ex);
			double c2y = ey + 2.0 / 3.0 * (qy - ey);
			pdfioContentPathCurve(s->st, c1x, c1y, c2x, c2y, ex, ey);
			*curx = ex; *cury = ey; pi += 4;
		} else { /* 'C' */
			double c1x = pen_x + g->pts[pi],     c1y = -g->pts[pi + 1];
			double c2x = pen_x + g->pts[pi + 2], c2y = -g->pts[pi + 3];
			double ex  = pen_x + g->pts[pi + 4], ey  = -g->pts[pi + 5];
			pdfioContentPathCurve(s->st, c1x, c1y, c2x, c2y, ex, ey);
			*curx = ex; *cury = ey; pi += 6;
		}
	}
}

void fc_pdf_emit_text_as_path(fc_pdf_state *s, double x, double y,
                               const char *font_path, double size_px,
                               uint32_t rgba, double angle_deg, int align,
                               const char *text, size_t text_len)
{
	if (!text || text_len == 0 || !font_path) return;
	if (fc_pdf_transparent(rgba)) return;

	FT_Face face = fastchart_ft_face(font_path);
	if (!face) return;
	FT_UInt pix = (FT_UInt)(size_px + 0.5);
	if (pix < 1) pix = 1;
	if (FT_Set_Pixel_Sizes(face, 0, pix)) return;

	/* Pass 1: total advance for alignment (also primes the cache). */
	double total_w = 0.0;
	{
		const unsigned char *p = (const unsigned char *)text;
		const unsigned char *e = p + text_len;
		uint32_t cp;
		while ((p = fc_pdf_utf8_next(p, e, &cp))) {
			const fc_glyph_cache_entry *g =
			    fastchart_resolve_glyph(face, (uint16_t)pix, cp);
			if (g) total_w += g->advance_x_64 / 64.0;
		}
	}
	double shift = 0.0;
	if (align == FASTCHART_TARGET_ALIGN_CENTER) shift = -total_w / 2.0;
	else if (align == FASTCHART_TARGET_ALIGN_RIGHT) shift = -total_w;

	pdfioContentSave(s->st);
	set_fill(s, rgba);
	/* Glyphs are emitted with y negated (-gy) to stand upright in PDF's
	 * y-up space; that flip mirrors the rotation direction relative to
	 * fastchart's CCW-in-y-down convention, so the page rotation is
	 * -angle. Pivot at the anchor (x, y) and apply the alignment shift
	 * AFTER the rotate, along the rotated baseline — matching
	 * fc_svg_emit_text_as_path. (At angle 0 the shift folds into the
	 * translate and text is already correct.) */
	if (angle_deg != 0.0) {
		pdfioContentMatrixTranslate(s->st, x, FY(s, y));
		pdfioContentMatrixRotate(s->st, -angle_deg);
		if (shift != 0.0) pdfioContentMatrixTranslate(s->st, shift, 0.0);
	} else {
		pdfioContentMatrixTranslate(s->st, x + shift, FY(s, y));
	}

	double pen_x = 0.0, curx = 0.0, cury = 0.0;
	int emitted = 0;
	{
		const unsigned char *p = (const unsigned char *)text;
		const unsigned char *e = p + text_len;
		uint32_t cp;
		while ((p = fc_pdf_utf8_next(p, e, &cp))) {
			const fc_glyph_cache_entry *g =
			    fastchart_resolve_glyph(face, (uint16_t)pix, cp);
			if (!g) continue;
			double adv = g->advance_x_64 / 64.0;
			if (g->n_ops > 0) {
				fc_pdf_replay_glyph(s, g, pen_x, &curx, &cury);
				emitted = 1;
			}
			pen_x += adv;
		}
	}
	if (emitted) pdfioContentFill(s->st, false);
	pdfioContentRestore(s->st);
}

/* ---- clip ---------------------------------------------------------- */

void fc_pdf_emit_clip_open(fc_pdf_state *s, double x, double y,
                            double w, double h)
{
	/* Save graphics state, intersect the clip with this rect. The
	 * matching clip_close restores, dropping the clip. PDF clip is set
	 * by constructing a path then the W operator; n ends the path
	 * without painting. */
	pdfioContentSave(s->st);
	pdfioContentPathRect(s->st, x, FY(s, y + h), w, h);
	pdfioContentClip(s->st, false);
	pdfioContentPathEnd(s->st);
}

void fc_pdf_emit_clip_close(fc_pdf_state *s)
{
	pdfioContentRestore(s->st);
}

/* ---- gradient (solid fallback, see header) ------------------------- */

void fc_pdf_emit_gradient_rect(fc_pdf_state *s, double x, double y,
                                double w, double h, uint32_t from_rgb,
                                uint32_t to_rgb, int dir)
{
	(void)to_rgb; (void)dir;
	/* No axial shading in this pdfio build — solid fallback of from_rgb.
	 * When the caller packed a non-zero alpha into the high byte (AreaChart
	 * band mode, fastchart_area.c), honor it so the fill composites against
	 * the captured page background instead of rendering opaque; a zero high
	 * byte is bare opaque RGB (stacked areas, plain gradients). */
	uint32_t color = (from_rgb & 0xFF000000u) ? from_rgb
	                                          : (from_rgb | 0xFF000000u);
	fc_pdf_emit_rect(s, x, y, w, h, color, 1, 0);
}

void fc_pdf_emit_gradient_polygon(fc_pdf_state *s, const int *xs,
                                   const int *ys, int n, uint32_t from_rgb,
                                   uint32_t to_rgb, int dir)
{
	(void)to_rgb; (void)dir;
	uint32_t color = (from_rgb & 0xFF000000u) ? from_rgb
	                                          : (from_rgb | 0xFF000000u);
	fc_pdf_emit_polygon(s, xs, ys, n, color, 1, 0);
}

#endif /* HAVE_FASTCHART_PDF */
