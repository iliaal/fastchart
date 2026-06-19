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

#include <math.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

/* Chord diagram: nodes are arc segments around a circle, sized by their
 * total incident flow; links are ribbons whose ends attach to value-
 * proportional slices of each endpoint's arc and curve through the
 * centre. Ribbon edges are quadratic beziers (control point = centre)
 * flattened to a polygon — same flatten-to-polygon strategy SankeyChart
 * uses for its horizontal ribbons. */

typedef struct {
    double incident;       /* sum of link values touching this node */
    double span_start;     /* arc start angle (deg, 0=east, CW) */
    double span_size;      /* arc angular width (deg) */
    double cursor;         /* consumed degrees within the span */
} chord_node_layout;

static inline void chord_pt(double cx, double cy, double r, double deg,
                            double *x, double *y)
{
    double rad = deg * M_PI / 180.0;
    *x = cx + r * cos(rad);
    *y = cy + r * sin(rad);
}

/* Append samples of the arc at radius r from deg0 to deg1 (inclusive). */
static int chord_arc_samples(double cx, double cy, double r,
                             double deg0, double deg1,
                             fastchart_point_t *out, int n_samp)
{
    for (int i = 0; i < n_samp; i++) {
        double d = deg0 + (deg1 - deg0) * ((double)i / (n_samp - 1));
        double x, y;
        chord_pt(cx, cy, r, d, &x, &y);
        out[i].x = (int)(x + 0.5);
        out[i].y = (int)(y + 0.5);
    }
    return n_samp;
}

/* Append samples of a quadratic bezier P0 -> Pc -> P1. */
static int chord_bezier_samples(double x0, double y0, double xc, double yc,
                                double x1, double y1,
                                fastchart_point_t *out, int n_samp)
{
    for (int i = 0; i < n_samp; i++) {
        double t = (double)i / (n_samp - 1);
        double mt = 1.0 - t;
        double bx = mt*mt*x0 + 2*mt*t*xc + t*t*x1;
        double by = mt*mt*y0 + 2*mt*t*yc + t*t*y1;
        out[i].x = (int)(bx + 0.5);
        out[i].y = (int)(by + 0.5);
    }
    return n_samp;
}

int fastchart_chord_render_to_target(fastchart_chord_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (self->node_count <= 0 || self->link_count <= 0) {
        zend_throw_error(NULL,
            "FastChart\\ChordDiagram::draw() requires both setNodes() and setLinks()");
        return -1;
    }

    int top_pad = 16;
    int title_h = 0;
    const char *title_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_TITLE);
    double base_size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double title_size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_TITLE, base_size * 1.4);
    if (self->title && ZSTR_LEN(self->title) > 0 && title_font) {
        if (fastchart_text_measure(t, title_font, title_size, ZSTR_VAL(self->title),
                                   NULL, &title_h, NULL, 0) == 0) {
            top_pad += title_h + 10;
        }
    }

    chord_node_layout *L = ecalloc(self->node_count, sizeof(*L));
    double total_flow = 0.0;
    int active = 0;
    for (int e = 0; e < self->link_count; e++) {
        L[self->links[e].from].incident += self->links[e].value;
        L[self->links[e].to].incident   += self->links[e].value;
        total_flow += 2.0 * self->links[e].value;
    }
    for (int i = 0; i < self->node_count; i++) {
        if (L[i].incident > 0.0) active++;
    }
    if (total_flow <= 0.0 || active == 0) {
        efree(L);
        zend_throw_error(NULL, "FastChart\\ChordDiagram::draw() has zero flow total");
        return -1;
    }

    double pad = self->pad_deg;
    if (pad < 0.0) pad = 0.0;
    if (pad * active > 300.0) pad = 300.0 / active;   /* keep arcs visible */
    double avail = 360.0 - pad * active;
    if (avail < 1.0) avail = 1.0;

    /* Walk the circle assigning each active node its arc span. */
    double ang = -90.0;   /* start at top */
    for (int i = 0; i < self->node_count; i++) {
        if (L[i].incident <= 0.0) { L[i].span_size = 0.0; continue; }
        double size = L[i].incident / total_flow * avail;
        L[i].span_start = ang;
        L[i].span_size = size;
        L[i].cursor = 0.0;
        ang += size + pad;
    }

    double cx = W / 2.0;
    double cy = top_pad + (H - top_pad) / 2.0;
    double usable = (double)(W < (H - top_pad) ? W : (H - top_pad));
    double label_margin = 64.0;
    double R = usable / 2.0 - label_margin;
    if (R < 20.0) R = 20.0;
    double band = 14.0;
    double R_in = R - band / 2.0 - 2.0;     /* ribbon attach radius */
    if (R_in < 10.0) R_in = 10.0;

    double max_val = 0.0;
    for (int e = 0; e < self->link_count; e++) {
        if (self->links[e].value > max_val) max_val = self->links[e].value;
    }
    if (max_val <= 0.0) max_val = 1.0;
    int ribbon_mode = self->style != FASTCHART_CHORD_STYLE_LINE;

    /* Ribbons (or curves) first, behind the node bands. Fills are
     * translucent so overlapping chords blend instead of one masking
     * the next. */
    fastchart_point_t poly[80];
    for (int e = 0; e < self->link_count; e++) {
        const fastchart_graph_link *lk = &self->links[e];
        chord_node_layout *a = &L[lk->from];
        chord_node_layout *b = &L[lk->to];
        double wa = lk->value / a->incident * a->span_size;
        double wb = lk->value / b->incident * b->span_size;
        double aL = a->span_start + a->cursor;
        double aR = aL + wa;
        double bL = b->span_start + b->cursor;
        double bR = bL + wb;
        a->cursor += wa;
        b->cursor += wb;

        int src_rgb;
        if (self->nodes[lk->from].color_rgb >= 0) {
            src_rgb = self->nodes[lk->from].color_rgb;
        } else {
            uint32_t rgba = fastchart_target_color_to_rgba(
                t, pal.series[lk->from % FASTCHART_PALETTE_SERIES_N]);
            src_rgb = (int)(rgba & 0xFFFFFF);
        }
        int cr = (src_rgb >> 16) & 0xFF, cg = (src_rgb >> 8) & 0xFF, cb = src_rgb & 0xFF;
        double x0, y0, x1, y1;

        if (ribbon_mode) {
            int n = 0;
            int k_arc = 5, k_bez = 14;
            n += chord_arc_samples(cx, cy, R_in, aL, aR, poly + n, k_arc);
            chord_pt(cx, cy, R_in, aR, &x0, &y0);
            chord_pt(cx, cy, R_in, bL, &x1, &y1);
            n += chord_bezier_samples(x0, y0, cx, cy, x1, y1, poly + n, k_bez);
            n += chord_arc_samples(cx, cy, R_in, bL, bR, poly + n, k_arc);
            chord_pt(cx, cy, R_in, bR, &x0, &y0);
            chord_pt(cx, cy, R_in, aL, &x1, &y1);
            n += chord_bezier_samples(x0, y0, cx, cy, x1, y1, poly + n, k_bez);
            int fill = fastchart_target_color(t, cr, cg, cb, 150);
            if (n >= 4) fastchart_target_polygon(t, poly, n, fill, 1, 0);
        } else {
            /* Non-ribbon: a single curve from the centre of each
             * endpoint's slice, stroke width proportional to value. */
            double amid = (aL + aR) / 2.0, bmid = (bL + bR) / 2.0;
            chord_pt(cx, cy, R_in, amid, &x0, &y0);
            chord_pt(cx, cy, R_in, bmid, &x1, &y1);
            int n = chord_bezier_samples(x0, y0, cx, cy, x1, y1, poly,
                                         (int)(sizeof(poly) / sizeof(poly[0])));
            int thickness = (int)(1.0 + (lk->value / max_val) * 8.0);
            if (thickness < 1) thickness = 1;
            int stroke = fastchart_target_color(t, cr, cg, cb, 200);
            /* One <polyline> per link instead of n-1 <line> elements: at
             * the link cap this is the difference between a few thousand
             * SVG elements and a few hundred thousand. */
            fastchart_target_polyline(t, poly, n, stroke, thickness);
        }
    }

    /* Node bands (thick arc strokes) + labels. */
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);
    for (int i = 0; i < self->node_count; i++) {
        if (L[i].span_size <= 0.0) continue;
        int color = self->nodes[i].color_rgb >= 0
            ? fastchart_target_color_rgb(t, self->nodes[i].color_rgb)
            : pal.series[i % FASTCHART_PALETTE_SERIES_N];
        fastchart_target_arc(t, (int)cx, (int)cy, (int)R, (int)R,
                             L[i].span_start, L[i].span_start + L[i].span_size,
                             color, 0, (int)band);
        if (font && self->nodes[i].label) {
            double mid = L[i].span_start + L[i].span_size / 2.0;
            double lx, ly;
            chord_pt(cx, cy, R + band / 2.0 + 6.0, mid, &lx, &ly);
            double mrad = mid * M_PI / 180.0;
            int align = cos(mrad) >= 0 ? FASTCHART_ALIGN_LEFT : FASTCHART_ALIGN_RIGHT;
            fastchart_text_draw(t, font, size, pal.text,
                                (int)lx, (int)(ly + size * 0.35), align,
                                self->nodes[i].label, NULL, 0);
        }
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    efree(L);
    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
