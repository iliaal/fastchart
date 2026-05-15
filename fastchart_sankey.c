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
#include <stdio.h>
#include <string.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

/* Sankey layout state per node. Cached for one render pass. */
typedef struct {
    int    layer;          /* topological column, 0..max_layer */
    double in_total;       /* sum of incoming flow */
    double out_total;      /* sum of outgoing flow */
    double y0;             /* top of node rect (pixels) */
    double height;         /* node rect height (pixels) */
    double in_cursor;      /* y of next inbound ribbon attachment */
    double out_cursor;     /* y of next outbound ribbon attachment */
} sankey_node_layout;

/* Compute per-node topological layer: longest path from any source.
 * Sources (no incoming) sit at layer 0. */
static void fastchart_sankey_compute_layers(
    const fastchart_sankey_link *links, int link_count,
    sankey_node_layout *L, int node_count, int *max_layer_out)
{
    for (int i = 0; i < node_count; i++) L[i].layer = 0;
    /* Iterate until stable. With a DAG this converges in
     * node_count passes; with cycles we cap at node_count and let
     * back-edges land on the latest stable assignment. */
    for (int pass = 0; pass < node_count; pass++) {
        int changed = 0;
        for (int e = 0; e < link_count; e++) {
            int target = L[links[e].from].layer + 1;
            if (target > L[links[e].to].layer) {
                L[links[e].to].layer = target;
                changed = 1;
            }
        }
        if (!changed) break;
    }
    int mx = 0;
    for (int i = 0; i < node_count; i++) {
        if (L[i].layer > mx) mx = L[i].layer;
    }
    *max_layer_out = mx;
}

/* Cubic Bezier path between two attachment points, expressed as a
 * pair of curves (top edge + bottom edge of the ribbon) flattened
 * to a polygon. Returns vertex count. */
static int fastchart_sankey_ribbon_polygon(
    double x0, double y0_top, double y0_bot,
    double x1, double y1_top, double y1_bot,
    fastchart_point_t *out, int max_pts)
{
    int seg = 24;
    if (2 * (seg + 1) > max_pts) seg = max_pts / 2 - 1;
    double cx0 = x0 + (x1 - x0) * 0.5;
    double cx1 = x0 + (x1 - x0) * 0.5;
    int n = 0;
    /* Top edge (left → right). */
    for (int i = 0; i <= seg; i++) {
        double tt = (double)i / seg;
        double mt = 1.0 - tt;
        double bx = mt*mt*mt*x0
            + 3*mt*mt*tt*cx0
            + 3*mt*tt*tt*cx1
            + tt*tt*tt*x1;
        double by = mt*mt*mt*y0_top
            + 3*mt*mt*tt*y0_top
            + 3*mt*tt*tt*y1_top
            + tt*tt*tt*y1_top;
        out[n].x = (int)(bx + 0.5);
        out[n].y = (int)(by + 0.5);
        n++;
    }
    /* Bottom edge (right → left). */
    for (int i = seg; i >= 0; i--) {
        double tt = (double)i / seg;
        double mt = 1.0 - tt;
        double bx = mt*mt*mt*x0
            + 3*mt*mt*tt*cx0
            + 3*mt*tt*tt*cx1
            + tt*tt*tt*x1;
        double by = mt*mt*mt*y0_bot
            + 3*mt*mt*tt*y0_bot
            + 3*mt*tt*tt*y1_bot
            + tt*tt*tt*y1_bot;
        out[n].x = (int)(bx + 0.5);
        out[n].y = (int)(by + 0.5);
        n++;
    }
    return n;
}

int fastchart_sankey_render_to_target(fastchart_sankey_obj *self, fastchart_target_t *t)
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
            "FastChart\\SankeyChart::draw() requires both setNodes() and setLinks()");
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

    sankey_node_layout *L = ecalloc(self->node_count, sizeof(*L));
    int max_layer = 0;
    fastchart_sankey_compute_layers(
        self->links, self->link_count, L, self->node_count, &max_layer);
    for (int e = 0; e < self->link_count; e++) {
        L[self->links[e].from].out_total += self->links[e].value;
        L[self->links[e].to].in_total    += self->links[e].value;
    }

    /* Per-node "size" = max(in, out). Use the largest column total
     * to derive a px-per-unit ratio that fits the plot vertically. */
    double col_max = 0.0;
    int n_layers = max_layer + 1;
    double *col_totals = ecalloc(n_layers, sizeof(*col_totals));
    int    *col_counts = ecalloc(n_layers, sizeof(*col_counts));
    for (int i = 0; i < self->node_count; i++) {
        double sz = L[i].in_total > L[i].out_total
            ? L[i].in_total : L[i].out_total;
        col_totals[L[i].layer] += sz;
        col_counts[L[i].layer] += 1;
        if (col_totals[L[i].layer] > col_max) col_max = col_totals[L[i].layer];
    }
    if (col_max <= 0) {
        efree(L); efree(col_totals); efree(col_counts);
        zend_throw_error(NULL,
            "FastChart\\SankeyChart::draw() has zero flow total");
        return -1;
    }

    /* Right margin sized for the rightmost column's labels; left
     * margin sized for the leftmost column's labels. Both columns
     * use the OUTER side of their node bars for text, so the inner
     * plot region stays clear of label collisions with ribbons. */
    int plot_x0 = 140, plot_x1 = W - 140;
    int plot_y0 = top_pad + 12, plot_y1 = H - 16;
    int avail_h = plot_y1 - plot_y0;
    int node_gap = 8;
    /* Reserve gap space for the most crowded column. */
    int max_count = 1;
    for (int c = 0; c < n_layers; c++) {
        if (col_counts[c] > max_count) max_count = col_counts[c];
    }
    double px_per_unit = (avail_h - (max_count - 1) * node_gap) / col_max;
    if (px_per_unit < 0.0) px_per_unit = 1.0;

    /* Position nodes column by column, top-aligned. */
    double col_x_step = (double)(plot_x1 - plot_x0) / (n_layers > 1 ? n_layers - 1 : 1);
    int node_w = 14;
    for (int c = 0; c < n_layers; c++) {
        double y = plot_y0;
        for (int i = 0; i < self->node_count; i++) {
            if (L[i].layer != c) continue;
            double sz = L[i].in_total > L[i].out_total
                ? L[i].in_total : L[i].out_total;
            double h = sz * px_per_unit;
            if (h < 2) h = 2;
            L[i].y0 = y;
            L[i].height = h;
            L[i].in_cursor = y;
            L[i].out_cursor = y;
            y += h + node_gap;
        }
    }

    /* Ribbons (drawn first, behind node rects). Color tinted from
     * source node's color or palette[from%N]. */
    fastchart_point_t ribbon_pts[64];
    for (int e = 0; e < self->link_count; e++) {
        const fastchart_sankey_link *lk = &self->links[e];
        sankey_node_layout *src = &L[lk->from];
        sankey_node_layout *dst = &L[lk->to];
        double h = lk->value * px_per_unit;
        double sx = plot_x0 + col_x_step * src->layer + node_w;
        double dx = plot_x0 + col_x_step * dst->layer;
        double s_top = src->out_cursor;
        double s_bot = src->out_cursor + h;
        double d_top = dst->in_cursor;
        double d_bot = dst->in_cursor + h;
        src->out_cursor += h;
        dst->in_cursor  += h;
        int npts = fastchart_sankey_ribbon_polygon(
            sx, s_top, s_bot, dx, d_top, d_bot,
            ribbon_pts, (int)(sizeof(ribbon_pts) / sizeof(ribbon_pts[0])));
        int color;
        if (self->nodes[lk->from].color_rgb >= 0) {
            color = fastchart_target_color_rgb(t, self->nodes[lk->from].color_rgb);
        } else {
            color = pal.series[lk->from % FASTCHART_PALETTE_SERIES_N];
        }
        if (npts >= 4) {
            fastchart_target_polygon(t, ribbon_pts, npts, color, 1, 0);
        }
    }

    /* Node rects + labels. */
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);
    for (int i = 0; i < self->node_count; i++) {
        int color = self->nodes[i].color_rgb >= 0
            ? fastchart_target_color_rgb(t, self->nodes[i].color_rgb)
            : pal.series[i % FASTCHART_PALETTE_SERIES_N];
        int nx = (int)(plot_x0 + col_x_step * L[i].layer);
        int ny = (int)L[i].y0;
        int nh = (int)L[i].height;
        if (nh < 2) nh = 2;
        fastchart_target_rect(t, nx, ny, node_w, nh, color, 1, 0);
        fastchart_target_rect(t, nx, ny, node_w, nh, pal.border, 0, 1);
        if (font && self->nodes[i].label) {
            /* Leftmost column (layer 0): label to the LEFT of the
             * node, right-aligned. All other columns including the
             * rightmost: label to the RIGHT of the node, left-
             * aligned. Putting every interior label on the right
             * side keeps the visual flow direction consistent
             * (left-to-right) and avoids label/ribbon collisions
             * since the OUTER side of leftmost / rightmost columns
             * has no ribbons. */
            int label_x, align;
            if (L[i].layer == 0) {
                label_x = nx - 6;
                align = FASTCHART_ALIGN_RIGHT;
            } else {
                label_x = nx + node_w + 6;
                align = FASTCHART_ALIGN_LEFT;
            }
            int label_y = ny + nh / 2 + (int)(size * 0.4);
            fastchart_text_draw(t, font, size, pal.text,
                                label_x, label_y, align,
                                self->nodes[i].label, NULL, 0);
        }
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    efree(L);
    efree(col_totals);
    efree(col_counts);
    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
