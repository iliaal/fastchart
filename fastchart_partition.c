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

/* Partition: the shared fastchart_pack_node hierarchy (CirclePacking's parser)
 * drawn as nested rectangles. Depth maps to fixed-width bands; each node's
 * span on the value axis is subdivided among its children in proportion to
 * their leaf-value sums. ORIENT_VERTICAL turns it into an icicle. Only this
 * layout/draw pass is partition-specific — parse / free / clone are reused. */

/* Subtree total = sum of leaf values (a leaf contributes its own value,
 * clamped to >= 0). Also tracks the deepest level seen. */
static double partition_total(const fastchart_pack_node *node, int depth,
                              int *max_depth)
{
    if (depth > *max_depth) *max_depth = depth;
    if (node->child_count == 0 || !node->children) {
        return node->value > 0.0 ? node->value : 0.0;
    }
    double sum = 0.0;
    for (int i = 0; i < node->child_count; i++) {
        sum += partition_total(node->children[i], depth + 1, max_depth);
    }
    return sum;
}

typedef struct {
    fastchart_target_t *t;
    int plot_x0, plot_y0, plot_w, plot_h;
    int orient;
    int max_depth;
    fastchart_palette *pal;
    const char *font;
    double font_size;
} partition_ctx;

/* Draw `node` occupying the value-axis pixel span [s0, s1] at `depth`, then
 * recurse into its children, splitting that span by leaf-value proportion. */
static void partition_draw(partition_ctx *c, fastchart_pack_node *node,
                           int depth, double s0, double s1, double node_total)
{
    double band = (c->orient == FASTCHART_PARTITION_ORIENT_VERTICAL)
        ? (double)c->plot_h / (double)(c->max_depth + 1)
        : (double)c->plot_w / (double)(c->max_depth + 1);

    int x, y, w, h;
    if (c->orient == FASTCHART_PARTITION_ORIENT_VERTICAL) {
        /* depth -> y rows (icicle); value span -> x. */
        x = (int)s0;
        y = c->plot_y0 + (int)(depth * band);
        w = (int)(s1 - s0);
        h = (int)band;
    } else {
        /* depth -> x columns; value span -> y. */
        x = c->plot_x0 + (int)(depth * band);
        y = (int)s0;
        w = (int)band;
        h = (int)(s1 - s0);
    }
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    int fill = node->color_rgb >= 0
        ? fastchart_target_color_rgb(c->t, node->color_rgb)
        : c->pal->series[depth % FASTCHART_PALETTE_SERIES_N];
    fastchart_target_rect(c->t, x, y, w, h, fill, 1, 0);
    fastchart_target_rect(c->t, x, y, w, h, c->pal->border, 0, 1);

    /* Label only when the cell is comfortably larger than the glyphs. */
    if (c->font && node->label && w > 14 && h > (int)(c->font_size * 1.2)) {
        fastchart_text_draw(c->t, c->font, c->font_size, c->pal->text,
                            x + w / 2, y + h / 2 + (int)(c->font_size * 0.35),
                            FASTCHART_ALIGN_CENTER, node->label, NULL, 0);
    }

    if (node->child_count == 0 || !node->children || node_total <= 0.0) {
        return;
    }
    double cur = s0;
    double span = s1 - s0;
    for (int i = 0; i < node->child_count; i++) {
        int cd = 0;
        double ct = partition_total(node->children[i], depth + 1, &cd);
        double frac = node_total > 0.0 ? ct / node_total : 0.0;
        double next = cur + frac * span;
        partition_draw(c, node->children[i], depth + 1, cur, next, ct);
        cur = next;
    }
}

int fastchart_partition_render_to_target(fastchart_partition_obj *self,
                                         fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_paint_canvas_bg(t, (fastchart_obj *)self, &pal);

    if (!self->root) {
        zend_throw_error(NULL,
            "FastChart\\Partition::draw() requires setHierarchy()");
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

    int max_depth = 0;
    double root_total = partition_total(self->root, 0, &max_depth);

    int plot_x0 = 16, plot_x1 = W - 16;
    int plot_y0 = top_pad + 8, plot_y1 = H - 16;
    fastchart_apply_plot_rect((fastchart_obj *)self,
                              &plot_x0, &plot_y0, &plot_x1, &plot_y1);
    if (plot_x1 - plot_x0 < 20 || plot_y1 - plot_y0 < 20) return 0;

    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);

    partition_ctx c;
    c.t = t;
    c.plot_x0 = plot_x0;
    c.plot_y0 = plot_y0;
    c.plot_w = plot_x1 - plot_x0;
    c.plot_h = plot_y1 - plot_y0;
    c.orient = (int)self->orientation;
    c.max_depth = max_depth;
    c.pal = &pal;
    c.font = font;
    c.font_size = size;

    /* The root spans the full value axis. If every value is zero the root
     * still draws as one band so the chart isn't blank. */
    double s0 = (c.orient == FASTCHART_PARTITION_ORIENT_VERTICAL)
        ? (double)plot_x0 : (double)plot_y0;
    double s1 = (c.orient == FASTCHART_PARTITION_ORIENT_VERTICAL)
        ? (double)plot_x1 : (double)plot_y1;
    partition_draw(&c, self->root, 0, s0, s1, root_total);

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
