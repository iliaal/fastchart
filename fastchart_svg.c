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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "Zend/zend_smart_str.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "fastchart_target.h"
#include "fastchart_svg.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Local: append a NUL-terminated literal to buf. Avoids one strlen
 * per call vs smart_str_appends. */
#define FC_APPENDS(buf, s) smart_str_appendl((buf), (s), sizeof(s) - 1)

void fc_svg_fmt_num(smart_str *buf, double v)
{
    /* %.1f covers chart geometry with no visible loss; trim "12.0"
     * to "12" but keep "12.5". Negative zero collapses to "0". */
    if (v == 0.0) { smart_str_appendc(buf, '0'); return; }
    char tmp[32];
    int n = snprintf(tmp, sizeof(tmp), "%.1f", v);
    if (n <= 0 || (size_t)n >= sizeof(tmp)) {
        smart_str_appends(buf, "0");
        return;
    }
    if (n >= 2 && tmp[n - 1] == '0' && tmp[n - 2] == '.') {
        n -= 2;
    }
    /* Normalise "-0" (which %.1f can emit for -0.05 rounded) to "0". */
    if (n == 2 && tmp[0] == '-' && tmp[1] == '0') {
        smart_str_appendc(buf, '0');
        return;
    }
    smart_str_appendl(buf, tmp, (size_t)n);
}

void fc_svg_fmt_color(smart_str *buf, uint32_t rgba)
{
    int a = (int)((rgba >> 24) & 0xFF);
    int r = (int)((rgba >> 16) & 0xFF);
    int g = (int)((rgba >>  8) & 0xFF);
    int b = (int)( rgba        & 0xFF);
    if (a == 0xFF) {
        char tmp[8];
        int n = snprintf(tmp, sizeof(tmp), "#%02X%02X%02X", r, g, b);
        if (n > 0) smart_str_appendl(buf, tmp, (size_t)n);
        return;
    }
    char tmp[40];
    int n = snprintf(tmp, sizeof(tmp), "rgba(%d,%d,%d,%.3f)",
                     r, g, b, (double)a / 255.0);
    if (n > 0) smart_str_appendl(buf, tmp, (size_t)n);
}

/* XML 1.0 PCDATA allows TAB (0x09), LF (0x0A), CR (0x0D), and Unicode
 * 0x20..0xD7FF / 0xE000..0xFFFD / 0x10000..0x10FFFF. Everything else
 * (C0 controls except TAB/LF/CR, surrogates, ill-formed UTF-8) makes
 * a conforming XML parser reject the document.
 *
 * fc_svg_escape walks the byte string and:
 *   - HTML-escapes the five XML metacharacters
 *   - validates UTF-8 byte sequences (length byte + continuation
 *     bytes + non-overlong + no surrogates)
 *   - replaces XML-invalid C0 bytes and any ill-formed UTF-8 with
 *     U+FFFD (UTF-8 EF BF BD)
 *
 * Embedded NUL is rejected at setter time so the loop normally
 * won't see one; the replacement path catches it defensively. */
static inline void fc_emit_run(smart_str *buf, const char *s,
                               size_t run_start, size_t i)
{
    if (i > run_start) {
        smart_str_appendl(buf, s + run_start, i - run_start);
    }
}

void fc_svg_escape(smart_str *buf, const char *s, size_t len)
{
    if (!s || len == 0) return;
    static const char REPL[] = "\xEF\xBF\xBD";  /* U+FFFD */
    size_t run_start = 0;
    size_t i = 0;
    while (i < len) {
        unsigned char c = (unsigned char)s[i];

        /* Fast path: ASCII printable (0x20..0x7E excluding the five
         * metacharacters) — keep walking, the trailing append flushes
         * the run. */
        if (c >= 0x20 && c < 0x80) {
            const char *esc = NULL;
            switch (c) {
                case '&':  esc = "&amp;";   break;
                case '<':  esc = "&lt;";    break;
                case '>':  esc = "&gt;";    break;
                case '"':  esc = "&quot;";  break;
                case '\'': esc = "&apos;";  break;
            }
            if (esc) {
                fc_emit_run(buf, s, run_start, i);
                smart_str_appends(buf, esc);
                run_start = ++i;
            } else {
                i++;
            }
            continue;
        }

        /* Allowed C0 controls — pass through. */
        if (c == 0x09 || c == 0x0A || c == 0x0D) { i++; continue; }

        /* C0 controls outside TAB/LF/CR -> U+FFFD. */
        if (c < 0x20) {
            fc_emit_run(buf, s, run_start, i);
            smart_str_appendl(buf, REPL, sizeof(REPL) - 1);
            run_start = ++i;
            continue;
        }

        /* UTF-8 multi-byte. Decode + validate the next sequence.
         *   2-byte: 110xxxxx 10xxxxxx        -> U+0080..U+07FF
         *   3-byte: 1110xxxx 10xxxxxx 10xxxxxx -> U+0800..U+FFFF
         *           (excluding surrogates U+D800..U+DFFF)
         *   4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx -> U+10000..U+10FFFF
         * Lone continuation bytes (0x80..0xBF leading) and the
         * disallowed C1 range (0xC0, 0xC1, 0xF5..0xFF) are invalid. */
        int n;
        uint32_t cp;
        uint32_t cp_min;
        if      (c < 0xC2)                  { n = 0; cp = 0; cp_min = 0; }
        else if (c < 0xE0) { n = 2; cp = c & 0x1F; cp_min = 0x80; }
        else if (c < 0xF0) { n = 3; cp = c & 0x0F; cp_min = 0x800; }
        else if (c < 0xF5) { n = 4; cp = c & 0x07; cp_min = 0x10000; }
        else               { n = 0; cp = 0; cp_min = 0; }

        if (n == 0 || i + (size_t)n > len) {
            fc_emit_run(buf, s, run_start, i);
            smart_str_appendl(buf, REPL, sizeof(REPL) - 1);
            run_start = ++i;
            continue;
        }

        int ok = 1;
        for (int k = 1; k < n; k++) {
            unsigned char cc = (unsigned char)s[i + k];
            if ((cc & 0xC0) != 0x80) { ok = 0; break; }
            cp = (cp << 6) | (cc & 0x3F);
        }
        if (!ok
            || cp < cp_min
            || (cp >= 0xD800 && cp <= 0xDFFF)
            || cp == 0xFFFE || cp == 0xFFFF
            || cp > 0x10FFFF) {
            fc_emit_run(buf, s, run_start, i);
            smart_str_appendl(buf, REPL, sizeof(REPL) - 1);
            /* Advance by one byte only — resync on the next start
             * byte rather than skipping `n` bytes of garbage. */
            run_start = ++i;
            continue;
        }

        i += (size_t)n;
    }
    fc_emit_run(buf, s, run_start, i);
}

void fc_svg_emit_doc_open(smart_str *buf, int width, int height)
{
    FC_APPENDS(buf,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
        "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "width=\"");
    smart_str_append_long(buf, width);
    FC_APPENDS(buf, "\" height=\"");
    smart_str_append_long(buf, height);
    FC_APPENDS(buf, "\" viewBox=\"0 0 ");
    smart_str_append_long(buf, width);
    smart_str_appendc(buf, ' ');
    smart_str_append_long(buf, height);
    FC_APPENDS(buf, "\">\n");
    /* Heatmap / Surface / Treemap / QrCode emit grids of adjacent
     * axis-aligned <rect> cells. SVG defaults rect edges to anti-
     * aliased rendering, which leaves a faint 1px gap of background
     * showing through between sub-pixel-aligned neighbours. Pin all
     * <rect> elements to crispEdges so cell boundaries are flush.
     * Chart frames, plot backgrounds, and bar series are also axis-
     * aligned, so crispEdges is a no-loss default for the family. */
    FC_APPENDS(buf, "<style>rect{shape-rendering:crispEdges}</style>\n");
}

void fc_svg_emit_doc_close(smart_str *buf)
{
    FC_APPENDS(buf, "</svg>\n");
}

void fc_svg_emit_g_open(smart_str *buf, const char *klass)
{
    if (klass && *klass) {
        FC_APPENDS(buf, "<g class=\"");
        fc_svg_escape(buf, klass, strlen(klass));
        FC_APPENDS(buf, "\">");
    } else {
        FC_APPENDS(buf, "<g>");
    }
}

void fc_svg_emit_g_close(smart_str *buf)
{
    FC_APPENDS(buf, "</g>\n");
}

/* Internal: append a stroke-dasharray="..." attribute for the given
 * dash pattern. dash == 0 emits nothing (solid). The patterns are
 * scaled by thickness so a thicker line gets longer dashes/gaps. */
static void fc_svg_emit_dash_attr(smart_str *buf, int dash, int thickness)
{
    if (dash == FASTCHART_DASH_SOLID) return;
    int t = thickness > 0 ? thickness : 1;
    FC_APPENDS(buf, " stroke-dasharray=\"");
    if (dash == FASTCHART_DASH_DOTTED) {
        smart_str_append_long(buf, t);
        smart_str_appendc(buf, ',');
        smart_str_append_long(buf, t * 2);
    } else {
        /* DASHED and any unknown */
        smart_str_append_long(buf, t * 4);
        smart_str_appendc(buf, ',');
        smart_str_append_long(buf, t * 3);
    }
    smart_str_appendc(buf, '"');
}

void fc_svg_emit_line(smart_str *buf,
                      double x0, double y0, double x1, double y1,
                      uint32_t rgba, int thickness, int dash)
{
    FC_APPENDS(buf, "<line x1=\"");
    fc_svg_fmt_num(buf, x0);
    FC_APPENDS(buf, "\" y1=\"");
    fc_svg_fmt_num(buf, y0);
    FC_APPENDS(buf, "\" x2=\"");
    fc_svg_fmt_num(buf, x1);
    FC_APPENDS(buf, "\" y2=\"");
    fc_svg_fmt_num(buf, y1);
    FC_APPENDS(buf, "\" stroke=\"");
    fc_svg_fmt_color(buf, rgba);
    smart_str_appendc(buf, '"');
    if (thickness > 1) {
        FC_APPENDS(buf, " stroke-width=\"");
        smart_str_append_long(buf, thickness);
        smart_str_appendc(buf, '"');
    }
    fc_svg_emit_dash_attr(buf, dash, thickness);
    FC_APPENDS(buf, "/>\n");
}

void fc_svg_emit_rect(smart_str *buf,
                      double x, double y, double w, double h,
                      uint32_t rgba, int fill, int thickness)
{
    FC_APPENDS(buf, "<rect x=\"");
    fc_svg_fmt_num(buf, x);
    FC_APPENDS(buf, "\" y=\"");
    fc_svg_fmt_num(buf, y);
    FC_APPENDS(buf, "\" width=\"");
    fc_svg_fmt_num(buf, w);
    FC_APPENDS(buf, "\" height=\"");
    fc_svg_fmt_num(buf, h);
    smart_str_appendc(buf, '"');
    if (fill) {
        FC_APPENDS(buf, " fill=\"");
        fc_svg_fmt_color(buf, rgba);
        smart_str_appendc(buf, '"');
    } else {
        FC_APPENDS(buf, " fill=\"none\" stroke=\"");
        fc_svg_fmt_color(buf, rgba);
        smart_str_appendc(buf, '"');
        if (thickness > 1) {
            FC_APPENDS(buf, " stroke-width=\"");
            smart_str_append_long(buf, thickness);
            smart_str_appendc(buf, '"');
        }
    }
    FC_APPENDS(buf, "/>\n");
}

void fc_svg_emit_polygon(smart_str *buf,
                          const int *xs, const int *ys, int n,
                          uint32_t rgba, int fill, int thickness)
{
    if (n < 2) return;
    FC_APPENDS(buf, "<polygon points=\"");
    for (int i = 0; i < n; i++) {
        if (i) smart_str_appendc(buf, ' ');
        smart_str_append_long(buf, xs[i]);
        smart_str_appendc(buf, ',');
        smart_str_append_long(buf, ys[i]);
    }
    smart_str_appendc(buf, '"');
    if (fill) {
        FC_APPENDS(buf, " fill=\"");
        fc_svg_fmt_color(buf, rgba);
        smart_str_appendc(buf, '"');
    } else {
        FC_APPENDS(buf, " fill=\"none\" stroke=\"");
        fc_svg_fmt_color(buf, rgba);
        smart_str_appendc(buf, '"');
        if (thickness > 1) {
            FC_APPENDS(buf, " stroke-width=\"");
            smart_str_append_long(buf, thickness);
            smart_str_appendc(buf, '"');
        }
    }
    FC_APPENDS(buf, "/>\n");
}

void fc_svg_emit_ellipse(smart_str *buf,
                          double cx, double cy, double rx, double ry,
                          uint32_t rgba, int fill, int thickness)
{
    if (rx == ry) {
        FC_APPENDS(buf, "<circle cx=\"");
        fc_svg_fmt_num(buf, cx);
        FC_APPENDS(buf, "\" cy=\"");
        fc_svg_fmt_num(buf, cy);
        FC_APPENDS(buf, "\" r=\"");
        fc_svg_fmt_num(buf, rx);
        smart_str_appendc(buf, '"');
    } else {
        FC_APPENDS(buf, "<ellipse cx=\"");
        fc_svg_fmt_num(buf, cx);
        FC_APPENDS(buf, "\" cy=\"");
        fc_svg_fmt_num(buf, cy);
        FC_APPENDS(buf, "\" rx=\"");
        fc_svg_fmt_num(buf, rx);
        FC_APPENDS(buf, "\" ry=\"");
        fc_svg_fmt_num(buf, ry);
        smart_str_appendc(buf, '"');
    }
    if (fill) {
        FC_APPENDS(buf, " fill=\"");
        fc_svg_fmt_color(buf, rgba);
        smart_str_appendc(buf, '"');
    } else {
        FC_APPENDS(buf, " fill=\"none\" stroke=\"");
        fc_svg_fmt_color(buf, rgba);
        smart_str_appendc(buf, '"');
        if (thickness > 1) {
            FC_APPENDS(buf, " stroke-width=\"");
            smart_str_append_long(buf, thickness);
            smart_str_appendc(buf, '"');
        }
    }
    FC_APPENDS(buf, "/>\n");
}

void fc_svg_emit_path_arc(smart_str *buf,
                           double cx, double cy, double rx, double ry,
                           double start_deg, double end_deg,
                           uint32_t rgba, int fill, int thickness)
{
    /* libgd's gdImageFilledArc has 0° at east, increasing clockwise.
     * SVG: x = cx + rx*cos(theta), y = cy + ry*sin(theta) in screen
     * coords where +y is south. sin(90°) = +1 (south), matching
     * libgd's CW convention. So no axis flip is needed; the two
     * coordinate systems agree. */
    double sweep = end_deg - start_deg;
    if (sweep < 0) sweep += 360.0;  /* normalise */

    /* Full-circle (or larger): SVG arc cannot draw a full circle in
     * one path command, so split into two semicircles. */
    if (sweep >= 359.999) {
        /* Draw as two half-arcs joined. For a filled wedge that's the
         * whole circle, just emit an <ellipse>. */
        if (fill) {
            fc_svg_emit_ellipse(buf, cx, cy, rx, ry, rgba, 1, 0);
            return;
        }
        /* Open arc full circle -> ellipse stroke. */
        fc_svg_emit_ellipse(buf, cx, cy, rx, ry, rgba, 0, thickness);
        return;
    }

    double s_rad = start_deg * M_PI / 180.0;
    double e_rad = end_deg   * M_PI / 180.0;
    double x0 = cx + rx * cos(s_rad);
    double y0 = cy + ry * sin(s_rad);
    double x1 = cx + rx * cos(e_rad);
    double y1 = cy + ry * sin(e_rad);
    int large = (sweep > 180.0) ? 1 : 0;
    int sweep_flag = 1;  /* CW to match gd */

    FC_APPENDS(buf, "<path d=\"");
    if (fill) {
        smart_str_appendc(buf, 'M');
        fc_svg_fmt_num(buf, cx);
        smart_str_appendc(buf, ',');
        fc_svg_fmt_num(buf, cy);
        FC_APPENDS(buf, " L");
        fc_svg_fmt_num(buf, x0);
        smart_str_appendc(buf, ',');
        fc_svg_fmt_num(buf, y0);
    } else {
        smart_str_appendc(buf, 'M');
        fc_svg_fmt_num(buf, x0);
        smart_str_appendc(buf, ',');
        fc_svg_fmt_num(buf, y0);
    }
    FC_APPENDS(buf, " A");
    fc_svg_fmt_num(buf, rx);
    smart_str_appendc(buf, ',');
    fc_svg_fmt_num(buf, ry);
    FC_APPENDS(buf, " 0 ");
    smart_str_append_long(buf, large);
    smart_str_appendc(buf, ',');
    smart_str_append_long(buf, sweep_flag);
    smart_str_appendc(buf, ' ');
    fc_svg_fmt_num(buf, x1);
    smart_str_appendc(buf, ',');
    fc_svg_fmt_num(buf, y1);
    if (fill) {
        FC_APPENDS(buf, " Z\"");
        FC_APPENDS(buf, " fill=\"");
        fc_svg_fmt_color(buf, rgba);
        smart_str_appendc(buf, '"');
    } else {
        smart_str_appendc(buf, '"');
        FC_APPENDS(buf, " fill=\"none\" stroke=\"");
        fc_svg_fmt_color(buf, rgba);
        smart_str_appendc(buf, '"');
        if (thickness > 1) {
            FC_APPENDS(buf, " stroke-width=\"");
            smart_str_append_long(buf, thickness);
            smart_str_appendc(buf, '"');
        }
    }
    FC_APPENDS(buf, "/>\n");
}

void fc_svg_emit_text(smart_str *buf,
                       double x, double y,
                       const char *family, double size_px,
                       uint32_t rgba, double angle_deg, int align,
                       const char *text, size_t text_len)
{
    if (!text || text_len == 0) return;
    FC_APPENDS(buf, "<text x=\"");
    fc_svg_fmt_num(buf, x);
    FC_APPENDS(buf, "\" y=\"");
    fc_svg_fmt_num(buf, y);
    FC_APPENDS(buf, "\" font-family=\"");
    /* family is pre-resolved and already attribute-safe per
     * fastchart_target_resolve_font_family; still pass through
     * escape in case a future caller bypasses the resolver. */
    fc_svg_escape(buf, family, strlen(family));
    FC_APPENDS(buf, ", sans-serif\" font-size=\"");
    fc_svg_fmt_num(buf, size_px);
    FC_APPENDS(buf, "\" fill=\"");
    fc_svg_fmt_color(buf, rgba);
    smart_str_appendc(buf, '"');
    if (align == FASTCHART_TARGET_ALIGN_CENTER) {
        FC_APPENDS(buf, " text-anchor=\"middle\"");
    } else if (align == FASTCHART_TARGET_ALIGN_RIGHT) {
        FC_APPENDS(buf, " text-anchor=\"end\"");
    }
    /* libgd / fastchart_text_draw_rotated rotates CCW; SVG rotate()
     * is CW with negative degrees for CCW. */
    if (angle_deg != 0.0) {
        FC_APPENDS(buf, " transform=\"rotate(");
        fc_svg_fmt_num(buf, -angle_deg);
        smart_str_appendc(buf, ' ');
        fc_svg_fmt_num(buf, x);
        smart_str_appendc(buf, ' ');
        fc_svg_fmt_num(buf, y);
        FC_APPENDS(buf, ")\"");
    }
    smart_str_appendc(buf, '>');
    fc_svg_escape(buf, text, text_len);
    FC_APPENDS(buf, "</text>\n");
}

/* --- Glyph-to-path text emission ---------------------------------- *
 *
 * Walks each codepoint, FT_Load_Glyphs the outline (unhinted, no
 * bitmap), and runs FT_Outline_Decompose with callbacks that
 * accumulate path-d data into a smart_str. Output goes inside one
 * <g transform="translate(x y) [rotate(-angle x y)]" fill="#rrggbb">
 * containing a single <path d="..."/> with all glyphs pre-translated
 * by their cumulative pen advance.
 *
 * Coordinate system flip: FreeType outlines have +y up. SVG has +y
 * down. We negate y in each point as we emit, so the path data is
 * directly in SVG coordinates and no scale(1,-1) wrapper is needed.
 *
 * Errors: any FT_* failure produces an empty <g> (nothing emitted).
 * The chart still renders; the text is simply absent. Loud errors
 * here would break otherwise-fine raster output.                       */

typedef struct {
	smart_str *buf;     /* destination */
	int        emitted; /* >0 once any contour wrote a Move */
	double     pen_x;   /* current pen advance in pixels (FT 26.6 / 64) */
	double     scale;   /* outline unit -> pixel; (1/64) for FT loaded at px */
	double     last_x;  /* last emitted point — used to skip degenerate */
	double     last_y;
} fc_path_emit_ctx;

static void fc_emit_num(smart_str *buf, double v)
{
	if (v == 0.0) { smart_str_appendc(buf, '0'); return; }
	char tmp[32];
	int n = snprintf(tmp, sizeof(tmp), "%.2f", v);
	if (n <= 0 || (size_t)n >= sizeof(tmp)) {
		smart_str_appendc(buf, '0'); return;
	}
	/* strip trailing zeros + trailing dot */
	while (n > 1 && tmp[n - 1] == '0') n--;
	if (n > 1 && tmp[n - 1] == '.') n--;
	smart_str_appendl(buf, tmp, n);
}

static int fc_path_move(const FT_Vector *to, void *u)
{
	fc_path_emit_ctx *c = u;
	double xx = c->pen_x + to->x * c->scale;
	double yy = -to->y * c->scale;
	smart_str_appendc(c->buf, 'M');
	fc_emit_num(c->buf, xx);
	smart_str_appendc(c->buf, ' ');
	fc_emit_num(c->buf, yy);
	c->last_x = xx; c->last_y = yy;
	c->emitted = 1;
	return 0;
}
static int fc_path_line(const FT_Vector *to, void *u)
{
	fc_path_emit_ctx *c = u;
	double xx = c->pen_x + to->x * c->scale;
	double yy = -to->y * c->scale;
	smart_str_appendc(c->buf, 'L');
	fc_emit_num(c->buf, xx);
	smart_str_appendc(c->buf, ' ');
	fc_emit_num(c->buf, yy);
	c->last_x = xx; c->last_y = yy;
	return 0;
}
static int fc_path_conic(const FT_Vector *ctrl, const FT_Vector *to, void *u)
{
	fc_path_emit_ctx *c = u;
	double cx = c->pen_x + ctrl->x * c->scale;
	double cy = -ctrl->y * c->scale;
	double xx = c->pen_x + to->x * c->scale;
	double yy = -to->y * c->scale;
	smart_str_appendc(c->buf, 'Q');
	fc_emit_num(c->buf, cx);
	smart_str_appendc(c->buf, ' ');
	fc_emit_num(c->buf, cy);
	smart_str_appendc(c->buf, ' ');
	fc_emit_num(c->buf, xx);
	smart_str_appendc(c->buf, ' ');
	fc_emit_num(c->buf, yy);
	c->last_x = xx; c->last_y = yy;
	return 0;
}
static int fc_path_cubic(const FT_Vector *c1, const FT_Vector *c2,
                          const FT_Vector *to, void *u)
{
	fc_path_emit_ctx *c = u;
	double x1 = c->pen_x + c1->x * c->scale;
	double y1 = -c1->y * c->scale;
	double x2 = c->pen_x + c2->x * c->scale;
	double y2 = -c2->y * c->scale;
	double xx = c->pen_x + to->x * c->scale;
	double yy = -to->y * c->scale;
	smart_str_appendc(c->buf, 'C');
	fc_emit_num(c->buf, x1); smart_str_appendc(c->buf, ' ');
	fc_emit_num(c->buf, y1); smart_str_appendc(c->buf, ' ');
	fc_emit_num(c->buf, x2); smart_str_appendc(c->buf, ' ');
	fc_emit_num(c->buf, y2); smart_str_appendc(c->buf, ' ');
	fc_emit_num(c->buf, xx); smart_str_appendc(c->buf, ' ');
	fc_emit_num(c->buf, yy);
	c->last_x = xx; c->last_y = yy;
	return 0;
}

static const FT_Outline_Funcs fc_path_funcs = {
	fc_path_move, fc_path_line, fc_path_conic, fc_path_cubic, 0, 0
};

/* Minimal inline UTF-8 -> codepoint walk. Returns next cursor or NULL
 * on truncation. *out_cp is set to the codepoint (replacement char on
 * invalid sequences). */
static const unsigned char *fc_utf8_next(const unsigned char *p,
                                          const unsigned char *end,
                                          uint32_t *out_cp)
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

/* FT_Library and FT_Face are both shared. The face cache (4-slot LRU
 * in fastchart_target.c) skips the FT_New_Face cost — opening a font
 * parses the entire file once. The size mutation (FT_Set_Pixel_Sizes)
 * still happens every call because callers want different sizes for
 * title vs axis labels; that's microseconds. */
void fc_svg_emit_text_as_path(smart_str *buf,
                               double x, double y,
                               const char *font_path, double size_px,
                               uint32_t rgba, double angle_deg, int align,
                               const char *text, size_t text_len)
{
	if (!text || text_len == 0 || !font_path) return;

	FT_Face face = fastchart_ft_face(font_path);
	if (!face) return;

	FT_UInt pix = (FT_UInt)(size_px + 0.5);
	if (pix < 1) pix = 1;
	if (FT_Set_Pixel_Sizes(face, 0, pix)) {
		return;
	}

	/* Pass 1: measure total advance for alignment. */
	double total_w = 0.0;
	{
		const unsigned char *p = (const unsigned char *)text;
		const unsigned char *e = p + text_len;
		uint32_t cp;
		while ((p = fc_utf8_next(p, e, &cp))) {
			FT_UInt gi = FT_Get_Char_Index(face, cp);
			if (FT_Load_Glyph(face, gi,
			                   FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING)) continue;
			total_w += face->glyph->advance.x / 64.0;
		}
	}

	double shift = 0.0;
	if (align == FASTCHART_TARGET_ALIGN_CENTER) shift = -total_w / 2.0;
	else if (align == FASTCHART_TARGET_ALIGN_RIGHT) shift = -total_w;

	/* Build a single combined path 'd' for all glyphs. Pen translates
	 * each glyph's outline by the cumulative advance. */
	smart_str d = {0};
	fc_path_emit_ctx ctx = {0};
	ctx.buf = &d;
	ctx.scale = 1.0 / 64.0;
	ctx.pen_x = 0.0;

	{
		const unsigned char *p = (const unsigned char *)text;
		const unsigned char *e = p + text_len;
		uint32_t cp;
		while ((p = fc_utf8_next(p, e, &cp))) {
			FT_UInt gi = FT_Get_Char_Index(face, cp);
			if (FT_Load_Glyph(face, gi,
			                   FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING)) continue;
			FT_Outline_Decompose(&face->glyph->outline, &fc_path_funcs, &ctx);
			ctx.pen_x += face->glyph->advance.x / 64.0;
		}
	}

	smart_str_0(&d);

	if (d.s && ZSTR_LEN(d.s) > 0) {
		FC_APPENDS(buf, "<g transform=\"translate(");
		fc_svg_fmt_num(buf, x + shift);
		smart_str_appendc(buf, ' ');
		fc_svg_fmt_num(buf, y);
		smart_str_appendc(buf, ')');
		if (angle_deg != 0.0) {
			/* libgd / fastchart_text_draw_rotated rotates CCW; SVG
			 * rotate() is CW. Apply at the post-translated origin
			 * (the alignment-shifted anchor), matching the existing
			 * <text> path. */
			FC_APPENDS(buf, " rotate(");
			fc_svg_fmt_num(buf, -angle_deg);
			FC_APPENDS(buf, ")");
		}
		FC_APPENDS(buf, "\" fill=\"");
		fc_svg_fmt_color(buf, rgba);
		FC_APPENDS(buf, "\"><path d=\"");
		smart_str_append_smart_str(buf, &d);
		FC_APPENDS(buf, "\"/></g>\n");
	}

	smart_str_free(&d);
	/* face is cache-owned; no FT_Done_Face here. */
}

void fc_svg_emit_clip_open(smart_str *buf, int id,
                            double x, double y, double w, double h)
{
    FC_APPENDS(buf, "<defs><clipPath id=\"fcc");
    smart_str_append_long(buf, id);
    FC_APPENDS(buf, "\"><rect x=\"");
    fc_svg_fmt_num(buf, x);
    FC_APPENDS(buf, "\" y=\"");
    fc_svg_fmt_num(buf, y);
    FC_APPENDS(buf, "\" width=\"");
    fc_svg_fmt_num(buf, w);
    FC_APPENDS(buf, "\" height=\"");
    fc_svg_fmt_num(buf, h);
    FC_APPENDS(buf, "\"/></clipPath></defs><g clip-path=\"url(#fcc");
    smart_str_append_long(buf, id);
    FC_APPENDS(buf, ")\">\n");
}

void fc_svg_emit_clip_close(smart_str *buf)
{
    FC_APPENDS(buf, "</g>\n");
}

void fc_svg_emit_image_uri(smart_str *buf, int x, int y, int w, int h,
                            const char *mime, const char *b64)
{
    if (!mime || !b64) return;
    FC_APPENDS(buf, "<image x=\"");
    smart_str_append_long(buf, x);
    FC_APPENDS(buf, "\" y=\"");
    smart_str_append_long(buf, y);
    FC_APPENDS(buf, "\" width=\"");
    smart_str_append_long(buf, w);
    FC_APPENDS(buf, "\" height=\"");
    smart_str_append_long(buf, h);
    FC_APPENDS(buf, "\" preserveAspectRatio=\"none\" href=\"data:");
    smart_str_appends(buf, mime);
    FC_APPENDS(buf, ";base64,");
    smart_str_appends(buf, b64);
    FC_APPENDS(buf, "\"/>\n");
}

/* Internal: emit a <linearGradient id="fcgN" ...> + two <stop>s.
 * x1/y1/x2/y2 are in percent units to keep userSpaceOnUse off; the
 * referenced shape applies the gradient to its own bbox. */
static void fc_svg_emit_gradient_def(smart_str *buf, int id,
                                      uint32_t from_rgb, uint32_t to_rgb,
                                      int dir)
{
    const char *x1, *y1, *x2, *y2;
    if (dir == 1) {
        x1 = "0%"; y1 = "0%"; x2 = "100%"; y2 = "0%";
    } else {
        x1 = "0%"; y1 = "0%"; x2 = "0%"; y2 = "100%";
    }
    /* Treat the high byte as alpha if the caller composed a full
     * 0xAARRGGBB; default to opaque when the caller passed a bare
     * 24-bit RGB (the common case from Bar/Pie). Translucent
     * gradients (AreaChart honoring setAreaAlpha) compose alpha
     * explicitly into the high byte. */
    uint32_t from_a = (from_rgb >> 24) & 0xFFu;
    if (from_a == 0) from_a = 0xFF;
    uint32_t to_a   = (to_rgb   >> 24) & 0xFFu;
    if (to_a   == 0) to_a   = 0xFF;
    uint32_t from = (from_a << 24) | (from_rgb & 0xFFFFFFu);
    uint32_t to   = (to_a   << 24) | (to_rgb   & 0xFFFFFFu);

    FC_APPENDS(buf, "<defs><linearGradient id=\"fcg");
    smart_str_append_long(buf, id);
    FC_APPENDS(buf, "\" x1=\"");
    smart_str_appends(buf, x1);
    FC_APPENDS(buf, "\" y1=\"");
    smart_str_appends(buf, y1);
    FC_APPENDS(buf, "\" x2=\"");
    smart_str_appends(buf, x2);
    FC_APPENDS(buf, "\" y2=\"");
    smart_str_appends(buf, y2);
    FC_APPENDS(buf, "\"><stop offset=\"0%\" stop-color=\"");
    fc_svg_fmt_color(buf, from);
    FC_APPENDS(buf, "\"/><stop offset=\"100%\" stop-color=\"");
    fc_svg_fmt_color(buf, to);
    FC_APPENDS(buf, "\"/></linearGradient></defs>");
}

void fc_svg_emit_gradient_rect(smart_str *buf, int id,
                                double x, double y, double w, double h,
                                uint32_t from_rgb, uint32_t to_rgb,
                                int dir)
{
    fc_svg_emit_gradient_def(buf, id, from_rgb, to_rgb, dir);
    FC_APPENDS(buf, "<rect x=\"");
    fc_svg_fmt_num(buf, x);
    FC_APPENDS(buf, "\" y=\"");
    fc_svg_fmt_num(buf, y);
    FC_APPENDS(buf, "\" width=\"");
    fc_svg_fmt_num(buf, w);
    FC_APPENDS(buf, "\" height=\"");
    fc_svg_fmt_num(buf, h);
    FC_APPENDS(buf, "\" fill=\"url(#fcg");
    smart_str_append_long(buf, id);
    FC_APPENDS(buf, ")\"/>\n");
}

void fc_svg_emit_gradient_polygon(smart_str *buf, int id,
                                   const int *xs, const int *ys, int n,
                                   uint32_t from_rgb, uint32_t to_rgb,
                                   int dir)
{
    if (n < 2) return;
    fc_svg_emit_gradient_def(buf, id, from_rgb, to_rgb, dir);
    FC_APPENDS(buf, "<polygon points=\"");
    for (int i = 0; i < n; i++) {
        if (i) smart_str_appendc(buf, ' ');
        smart_str_append_long(buf, xs[i]);
        smart_str_appendc(buf, ',');
        smart_str_append_long(buf, ys[i]);
    }
    FC_APPENDS(buf, "\" fill=\"url(#fcg");
    smart_str_append_long(buf, id);
    FC_APPENDS(buf, ")\"/>\n");
}
