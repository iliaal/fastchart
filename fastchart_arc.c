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

/* Arc diagram: nodes sit on a single baseline, each link is drawn as a
 * semicircular (elliptical for long spans) open arc connecting its two
 * endpoints. Link arcs bulge to one side of the baseline per the
 * orientation setting; node markers and labels sit on the opposite
 * side so they never collide with the ribbons. */

int fastchart_arc_render_to_target(fastchart_arc_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_paint_canvas_bg(t, (fastchart_obj *)self, &pal);

    if (self->node_count <= 0 || self->link_count <= 0) {
        zend_throw_error(NULL,
            "FastChart\\ArcDiagram::draw() requires both setNodes() and setLinks()");
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

    /* Per-node incident flow drives marker size; per-link value drives
     * arc stroke width. */
    double *incident = ecalloc(self->node_count, sizeof(*incident));
    double max_incident = 0.0, max_val = 0.0;
    for (int e = 0; e < self->link_count; e++) {
        const fastchart_graph_link *lk = &self->links[e];
        incident[lk->from] += lk->value;
        incident[lk->to]   += lk->value;
        if (lk->value > max_val) max_val = lk->value;
    }
    for (int i = 0; i < self->node_count; i++) {
        if (incident[i] > max_incident) max_incident = incident[i];
    }
    if (max_val <= 0.0) max_val = 1.0;
    if (max_incident <= 0.0) max_incident = 1.0;

    int side_pad = 40;
    int label_band = (int)(base_size * 3.0);
    int plot_x0 = side_pad, plot_x1 = W - side_pad;
    int plot_y0 = top_pad + 8, plot_y1 = H - 12;
    fastchart_apply_plot_rect((fastchart_obj *)self,
                              &plot_x0, &plot_y0, &plot_x1, &plot_y1);
    if (plot_x1 <= plot_x0) { efree(incident); return 0; }

    /* Baseline placement + available arc height per orientation. */
    double base_y;
    int arc_h_up, arc_h_dn;
    switch (self->orientation) {
        case FASTCHART_ARC_ORIENT_DOWN:
            base_y = plot_y0 + label_band;
            arc_h_up = 0;
            arc_h_dn = (int)(plot_y1 - base_y);
            break;
        case FASTCHART_ARC_ORIENT_SPLIT: {
            base_y = (plot_y0 + plot_y1) / 2.0;
            int up = (int)(base_y - plot_y0);
            int dn = (int)(plot_y1 - base_y) - label_band;
            arc_h_up = up;
            arc_h_dn = dn > 4 ? dn : 4;
            break;
        }
        case FASTCHART_ARC_ORIENT_UP:
        default:
            base_y = plot_y1 - label_band;
            arc_h_up = (int)(base_y - plot_y0);
            arc_h_dn = 0;
            break;
    }
    if (arc_h_up < 0) arc_h_up = 0;
    if (arc_h_dn < 0) arc_h_dn = 0;

    /* Even node spacing across the plot width. */
    double step = self->node_count > 1
        ? (double)(plot_x1 - plot_x0) / (self->node_count - 1)
        : 0.0;
    double *node_x = ecalloc(self->node_count, sizeof(*node_x));
    for (int i = 0; i < self->node_count; i++) {
        node_x[i] = self->node_count > 1
            ? plot_x0 + step * i
            : (plot_x0 + plot_x1) / 2.0;
    }

    /* Link arcs (drawn first, behind the node markers). */
    for (int e = 0; e < self->link_count; e++) {
        const fastchart_graph_link *lk = &self->links[e];
        double xa = node_x[lk->from], xb = node_x[lk->to];
        double cx = (xa + xb) / 2.0;
        double rx = fabs(xb - xa) / 2.0;
        if (rx < 1.0) rx = 1.0;

        int up;   /* arc bulges upward? */
        if (self->orientation == FASTCHART_ARC_ORIENT_DOWN) {
            up = 0;
        } else if (self->orientation == FASTCHART_ARC_ORIENT_SPLIT) {
            up = lk->to > lk->from;
        } else {
            up = 1;
        }
        int arc_h = up ? arc_h_up : arc_h_dn;
        double ry = rx < arc_h ? rx : arc_h;
        if (ry < 2.0) ry = 2.0;

        int color = self->nodes[lk->from].color_rgb >= 0
            ? fastchart_target_color_rgb(t, self->nodes[lk->from].color_rgb)
            : pal.series[lk->from % FASTCHART_PALETTE_SERIES_N];
        int thickness = (int)(1.0 + (lk->value / max_val) * 5.0);
        if (thickness < 1) thickness = 1;

        /* 0deg = east, increasing clockwise (screen y down). Top half
         * = 180->360 (through north); bottom half = 0->180. */
        double start_deg = up ? 180.0 : 0.0;
        double end_deg   = up ? 360.0 : 180.0;
        fastchart_target_arc(t, (int)cx, (int)base_y, (int)rx, (int)ry,
                             start_deg, end_deg, color, 0, thickness);
    }

    /* Node markers + labels. */
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);
    int labels_below = (self->orientation != FASTCHART_ARC_ORIENT_DOWN);
    for (int i = 0; i < self->node_count; i++) {
        int color = self->nodes[i].color_rgb >= 0
            ? fastchart_target_color_rgb(t, self->nodes[i].color_rgb)
            : pal.series[i % FASTCHART_PALETTE_SERIES_N];
        int r = (int)(3.0 + (incident[i] / max_incident) * 6.0);
        if (r < 3) r = 3;
        int cx = (int)node_x[i];
        int cy = (int)base_y;
        fastchart_target_ellipse(t, cx, cy, r, r, color, 1, 0);
        fastchart_target_ellipse(t, cx, cy, r, r, pal.border, 0, 1);

        if (font && self->nodes[i].label) {
            int ly = labels_below
                ? cy + r + (int)(size * 0.6)
                : cy - r - (int)(size * 0.2);
            fastchart_text_draw_rotated(t, font, size, pal.text,
                                        cx, ly, FASTCHART_ALIGN_RIGHT, 45.0,
                                        self->nodes[i].label, NULL, 0);
        }
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    efree(incident);
    efree(node_x);
    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
