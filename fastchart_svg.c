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

void fc_svg_escape(smart_str *buf, const char *s, size_t len)
{
    if (!s || len == 0) return;
    size_t run_start = 0;
    for (size_t i = 0; i < len; i++) {
        const char *esc = NULL;
        switch (s[i]) {
            case '&':  esc = "&amp;";  break;
            case '<':  esc = "&lt;";   break;
            case '>':  esc = "&gt;";   break;
            case '"':  esc = "&quot;"; break;
            case '\'': esc = "&apos;"; break;
            default: continue;
        }
        if (i > run_start) {
            smart_str_appendl(buf, s + run_start, i - run_start);
        }
        smart_str_appends(buf, esc);
        run_start = i + 1;
    }
    if (run_start < len) {
        smart_str_appendl(buf, s + run_start, len - run_start);
    }
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
