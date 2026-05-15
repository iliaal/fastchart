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

/* Each node gets a precomputed angular wedge [start, end] (radians)
 * based on its share of its parent's value. Root sits at depth 0;
 * leaves at max_depth. Rings sized so the outermost ring still
 * leaves room for the title. */
typedef struct {
    double start_rad;
    double end_rad;
} sunburst_wedge;

static void fastchart_sunburst_compute_wedges(
    const fastchart_sunburst_node *nodes, int node_count,
    sunburst_wedge *wedges)
{
    if (node_count <= 0) return;
    wedges[0].start_rad = -M_PI_2;
    wedges[0].end_rad   = -M_PI_2 + 2.0 * M_PI;
    /* BFS-style by depth: a parent's wedge is fully partitioned
     * among its children proportional to their values. Walk in node
     * order; since build_rec is DFS, every node's parent is already
     * processed when we encounter it. */
    for (int i = 0; i < node_count; i++) {
        if (nodes[i].child_count <= 0) continue;
        double sum = 0.0;
        for (int c = 0; c < nodes[i].child_count; c++) {
            sum += nodes[nodes[i].child_first + c].value;
        }
        if (sum <= 0) continue;
        double parent_span = wedges[i].end_rad - wedges[i].start_rad;
        double cur = wedges[i].start_rad;
        for (int c = 0; c < nodes[i].child_count; c++) {
            int ci = nodes[i].child_first + c;
            double share = nodes[ci].value / sum;
            wedges[ci].start_rad = cur;
            wedges[ci].end_rad   = cur + parent_span * share;
            cur = wedges[ci].end_rad;
        }
    }
}

/* Build a 4-point ring-arc polygon (a "quadrilateral" with the
 * inner / outer edges sampled along the arc). We approximate the
 * arc with a chord-fan: split the angular span into segments of at
 * most ~6 degrees so the polygon edges hug the true arc. */
static int fastchart_sunburst_ring_polygon(
    int cx, int cy, double r_inner, double r_outer,
    double a0, double a1,
    fastchart_point_t *out, int max_pts)
{
    int seg = (int)ceil((a1 - a0) / (M_PI / 30.0));
    if (seg < 1) seg = 1;
    /* Need 2*(seg+1) points: outer arc forward + inner arc backward. */
    int needed = 2 * (seg + 1);
    if (needed > max_pts) {
        seg = max_pts / 2 - 1;
        needed = 2 * (seg + 1);
    }
    int n = 0;
    for (int i = 0; i <= seg; i++) {
        double a = a0 + (a1 - a0) * i / seg;
        out[n].x = cx + (int)(r_outer * cos(a) + 0.5);
        out[n].y = cy + (int)(r_outer * sin(a) + 0.5);
        n++;
    }
    for (int i = seg; i >= 0; i--) {
        double a = a0 + (a1 - a0) * i / seg;
        out[n].x = cx + (int)(r_inner * cos(a) + 0.5);
        out[n].y = cy + (int)(r_inner * sin(a) + 0.5);
        n++;
    }
    return n;
}

int fastchart_sunburst_render_to_target(fastchart_sunburst_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (self->node_count <= 0 || self->total_value <= 0.0) {
        zend_throw_error(NULL,
            "FastChart\\SunburstChart::draw() requires setHierarchy() with positive values");
        return -1;
    }

    int top_pad = 12;
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

    int avail = W < (H - top_pad) ? W : (H - top_pad);
    int cx = W / 2;
    int cy = top_pad + (H - top_pad) / 2;
    int outer_r = (avail / 2) - 20;
    if (outer_r < 32) outer_r = 32;
    /* Innermost ring (depth 1) starts at a small radius so depth 0
     * (root) is a small donut hole carrying the root label. Ring
     * width tapers slightly toward the outside so all max_depth
     * rings fit inside outer_r. */
    int n_rings = self->max_depth;     /* depths 1..max_depth */
    if (n_rings <= 0) n_rings = 1;
    int inner_hole = outer_r / (n_rings + 2);
    double ring_w = (double)(outer_r - inner_hole) / (double)n_rings;

    sunburst_wedge *wedges = ecalloc(self->node_count, sizeof(*wedges));
    fastchart_sunburst_compute_wedges(self->nodes, self->node_count, wedges);

    /* Render outer rings first, then inner rings on top so wedge
     * borders draw cleanly. */
    int leaf_idx = 0;
    fastchart_point_t pts[128];
    for (int d = self->max_depth; d >= 1; d--) {
        double r0 = inner_hole + ring_w * (d - 1);
        double r1 = inner_hole + ring_w * d;
        for (int i = 1; i < self->node_count; i++) {
            if (self->nodes[i].depth != d) continue;
            int color;
            if (self->nodes[i].color_rgb >= 0) {
                color = fastchart_target_color_rgb(t, self->nodes[i].color_rgb);
            } else {
                color = pal.series[leaf_idx % FASTCHART_PALETTE_SERIES_N];
                leaf_idx++;
            }
            int npts = fastchart_sunburst_ring_polygon(
                cx, cy, r0, r1,
                wedges[i].start_rad, wedges[i].end_rad,
                pts, (int)(sizeof(pts) / sizeof(pts[0])));
            if (npts >= 4) {
                fastchart_target_polygon(t, pts, npts, color, 1, 0);
                fastchart_target_polygon(t, pts, npts, pal.border, 0, 1);
            }
        }
    }

    /* Root hub: a filled circle in the bg color with optional label. */
    fastchart_target_ellipse(t, cx, cy, inner_hole, inner_hole,
                             pal.bg, 1, 0);
    fastchart_target_ellipse(t, cx, cy, inner_hole, inner_hole,
                             pal.border, 0, 1);
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);
    if (font && self->node_count > 0 && self->nodes[0].label) {
        fastchart_text_draw(t, font, size, pal.text,
                            cx, cy + (int)(size * 0.4),
                            FASTCHART_ALIGN_CENTER,
                            self->nodes[0].label, NULL, 0);
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    efree(wedges);
    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
