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
#include <stdlib.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

/* Dendrogram: the shared fastchart_pack_node hierarchy (CirclePacking's
 * parser) drawn as a node-link tree. A tidy-tree layout assigns each leaf
 * a sequential cross position and centres each internal node over its
 * children; depth maps to the perpendicular axis. Only this layout/draw
 * pass is dendrogram-specific — parse / free / clone are reused verbatim. */

/* Post-order placement: store cross position in node->x, depth in node->y
 * (both abstract units). Returns the node's cross coordinate. */
static double dendro_layout(fastchart_pack_node *node, int depth,
                            int *next_leaf, int *max_depth)
{
    if (depth > *max_depth) *max_depth = depth;
    if (node->child_count == 0 || !node->children) {
        double cross = (double)((*next_leaf)++);
        node->x = cross;
        node->y = (double)depth;
        return cross;
    }
    double first = 0.0, last = 0.0;
    for (int i = 0; i < node->child_count; i++) {
        double c = dendro_layout(node->children[i], depth + 1,
                                 next_leaf, max_depth);
        if (i == 0) first = c;
        last = c;
    }
    node->x = (first + last) / 2.0;   /* centre over first..last child */
    node->y = (double)depth;
    return node->x;
}

typedef struct {
    fastchart_target_t *t;
    int plot_x0, plot_y0, plot_w, plot_h;
    double cross_max;   /* n_leaves - 1, may be 0 for a single leaf */
    double depth_max;   /* deepest level, may be 0 */
    int orient;
    int style;
    int edge_color;
    int node_color;
    int text_color;
    const char *font;
    double font_size;
} dendro_ctx;

/* Map a node's (cross, depth) to a pixel coordinate. cross/depth fractions
 * are clamped to [0,1] so the int casts stay inside the plot rect. */
static void dendro_pos(const dendro_ctx *c, const fastchart_pack_node *n,
                       int *px, int *py)
{
    double cf = c->cross_max > 0.0 ? n->x / c->cross_max : 0.5;
    double df = c->depth_max > 0.0 ? n->y / c->depth_max : 0.0;
    if (cf < 0.0) cf = 0.0; else if (cf > 1.0) cf = 1.0;
    if (df < 0.0) df = 0.0; else if (df > 1.0) df = 1.0;
    if (c->orient == FASTCHART_DENDRO_ORIENT_LEFT) {
        *px = c->plot_x0 + (int)(df * c->plot_w);
        *py = c->plot_y0 + (int)(cf * c->plot_h);
    } else {
        *px = c->plot_x0 + (int)(cf * c->plot_w);
        *py = c->plot_y0 + (int)(df * c->plot_h);
    }
}

static void dendro_draw(dendro_ctx *c, fastchart_pack_node *node)
{
    int px, py;
    dendro_pos(c, node, &px, &py);

    for (int i = 0; i < node->child_count; i++) {
        fastchart_pack_node *ch = node->children[i];
        int cx, cy;
        dendro_pos(c, ch, &cx, &cy);
        if (c->style == FASTCHART_DENDRO_STYLE_ELBOW) {
            /* Right-angle edge bending at the parent's depth level. */
            if (c->orient == FASTCHART_DENDRO_ORIENT_LEFT) {
                fastchart_target_line(c->t, px, py, px, cy,
                                      c->edge_color, 1, FASTCHART_DASH_SOLID);
                fastchart_target_line(c->t, px, cy, cx, cy,
                                      c->edge_color, 1, FASTCHART_DASH_SOLID);
            } else {
                fastchart_target_line(c->t, px, py, cx, py,
                                      c->edge_color, 1, FASTCHART_DASH_SOLID);
                fastchart_target_line(c->t, cx, py, cx, cy,
                                      c->edge_color, 1, FASTCHART_DASH_SOLID);
            }
        } else {
            fastchart_target_line(c->t, px, py, cx, cy,
                                  c->edge_color, 1, FASTCHART_DASH_SOLID);
        }
        dendro_draw(c, ch);
    }

    int col = node->color_rgb >= 0
        ? fastchart_target_color_rgb(c->t, node->color_rgb)
        : c->node_color;
    fastchart_target_ellipse(c->t, px, py, 4, 4, col, 1, 0);

    if (c->font && node->label) {
        fastchart_text_draw(c->t, c->font, c->font_size, c->text_color,
                            px, (int)(py - c->font_size * 0.7),
                            FASTCHART_ALIGN_CENTER, node->label, NULL, 0);
    }
}

int fastchart_dendrogram_render_to_target(fastchart_dendrogram_obj *self,
                                          fastchart_target_t *t)
{
    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_paint_canvas_bg(t, (fastchart_obj *)self, &pal);

    if (!self->root) {
        zend_throw_error(NULL,
            "FastChart\\Dendrogram::draw() requires setHierarchy()");
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

    int next_leaf = 0, max_depth = 0;
    dendro_layout(self->root, 0, &next_leaf, &max_depth);
    int n_leaves = next_leaf < 1 ? 1 : next_leaf;

    int plot_x0 = 40, plot_x1 = W - 40;
    int plot_y0 = top_pad + 12, plot_y1 = H - 24;
    fastchart_apply_plot_rect((fastchart_obj *)self,
                              &plot_x0, &plot_y0, &plot_x1, &plot_y1);
    if (plot_x1 - plot_x0 < 20 || plot_y1 - plot_y0 < 20) return 0;

    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);

    dendro_ctx c;
    c.t = t;
    c.plot_x0 = plot_x0;
    c.plot_y0 = plot_y0;
    c.plot_w = plot_x1 - plot_x0;
    c.plot_h = plot_y1 - plot_y0;
    c.cross_max = (double)(n_leaves - 1);
    c.depth_max = (double)max_depth;
    c.orient = (int)self->orientation;
    c.style = (int)self->style;
    c.edge_color = pal.axis;
    c.node_color = pal.series[0];
    c.text_color = pal.text;
    c.font = font;
    c.font_size = size;

    dendro_draw(&c, self->root);

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
