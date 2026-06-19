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

/* Circle packing: a hierarchy of nested circles. Leaves are sized by
 * value (radius proportional to sqrt(value) so area tracks value); each
 * parent is the enclosing circle of its spiral-packed children. The
 * spiral placement keeps siblings non-overlapping — the property that
 * actually matters visually — at the cost of a slightly loose
 * (non-minimal) enclosing circle. A fixed iteration cap bounds the
 * search; in the rare case it is exhausted the last candidate is used
 * as-is (see pack_siblings). Coordinates are kept abstract during
 * packing and scaled to the canvas in a single top-down render pass. */

#define PACK_PAD 2.0

static int pack_radius_cmp(const void *a, const void *b)
{
    const fastchart_pack_node *x = *(const fastchart_pack_node *const *)a;
    const fastchart_pack_node *y = *(const fastchart_pack_node *const *)b;
    return (x->r < y->r) - (x->r > y->r);   /* descending */
}

/* Place siblings (each already carrying its own radius) without overlap
 * by spiralling outward from the centre, largest first. Then recentre
 * on the enclosing circle's centre. Returns the enclosing radius. */
static double pack_siblings(fastchart_pack_node **kids, int n)
{
    if (n <= 0) return 0.0;
    if (n == 1) { kids[0]->x = 0.0; kids[0]->y = 0.0; return kids[0]->r; }

    qsort(kids, n, sizeof(*kids), pack_radius_cmp);

    double avg_r = 0.0;
    for (int i = 0; i < n; i++) avg_r += kids[i]->r;
    avg_r /= n;
    double rad_step = avg_r * 0.05;
    if (rad_step < 0.01) rad_step = 0.01;

    for (int i = 0; i < n; i++) {
        fastchart_pack_node *c = kids[i];
        if (i == 0) { c->x = 0.0; c->y = 0.0; continue; }
        /* Spiral outward until a collision-free slot is found. The cap is
         * a backstop; rad grows without bound so a clear slot is reached
         * long before it for any realistic radius set. If the cap is hit,
         * the last (possibly overlapping) candidate stands. */
        int placed = 0;
        for (long s = 0; s < 200000 && !placed; s++) {
            double tt = (double)s;
            double ang = tt * 2.39996322972865332;   /* golden angle */
            double rad = rad_step * tt;
            c->x = rad * cos(ang);
            c->y = rad * sin(ang);
            placed = 1;
            for (int j = 0; j < i; j++) {
                double dxp = c->x - kids[j]->x;
                double dyp = c->y - kids[j]->y;
                double need = c->r + kids[j]->r;
                if (dxp * dxp + dyp * dyp < need * need - 0.001) {
                    placed = 0;
                    break;
                }
            }
        }
    }

    /* Enclose: bbox centre, then max reach. */
    double minx = kids[0]->x - kids[0]->r, maxx = kids[0]->x + kids[0]->r;
    double miny = kids[0]->y - kids[0]->r, maxy = kids[0]->y + kids[0]->r;
    for (int i = 1; i < n; i++) {
        if (kids[i]->x - kids[i]->r < minx) minx = kids[i]->x - kids[i]->r;
        if (kids[i]->x + kids[i]->r > maxx) maxx = kids[i]->x + kids[i]->r;
        if (kids[i]->y - kids[i]->r < miny) miny = kids[i]->y - kids[i]->r;
        if (kids[i]->y + kids[i]->r > maxy) maxy = kids[i]->y + kids[i]->r;
    }
    double cx = (minx + maxx) / 2.0, cy = (miny + maxy) / 2.0;
    double R = 0.0;
    for (int i = 0; i < n; i++) {
        kids[i]->x -= cx;
        kids[i]->y -= cy;
        double reach = sqrt(kids[i]->x * kids[i]->x + kids[i]->y * kids[i]->y) + kids[i]->r;
        if (reach > R) R = reach;
    }
    return R;
}

/* Post-order: assign every node its packed radius. */
static void pack_layout(fastchart_pack_node *node)
{
    if (node->child_count == 0) {
        double v = node->value > 0.0 ? node->value : 0.0;
        node->r = sqrt(v);
        if (node->r < 1.0) node->r = 1.0;
        return;
    }
    for (int i = 0; i < node->child_count; i++) {
        pack_layout(node->children[i]);
    }
    double R = pack_siblings(node->children, node->child_count);
    node->r = R + PACK_PAD;
}

static void pack_draw(fastchart_target_t *t, fastchart_pack_node *node,
                      double cx, double cy, double scale,
                      fastchart_palette *pal, const char *font, double size,
                      int *leaf_idx)
{
    double r = node->r * scale;
    if (node->child_count == 0) {
        int color = node->color_rgb >= 0
            ? fastchart_target_color_rgb(t, node->color_rgb)
            : pal->series[(*leaf_idx)++ % FASTCHART_PALETTE_SERIES_N];
        fastchart_target_ellipse(t, (int)cx, (int)cy, (int)r, (int)r, color, 1, 0);
        fastchart_target_ellipse(t, (int)cx, (int)cy, (int)r, (int)r, pal->border, 0, 1);
        if (font && node->label && r > size * 1.2) {
            fastchart_text_draw(t, font, size, pal->text,
                                (int)cx, (int)(cy + size * 0.35),
                                FASTCHART_ALIGN_CENTER, node->label, NULL, 0);
        }
        return;
    }
    /* Internal: outline only so children stay visible. */
    fastchart_target_ellipse(t, (int)cx, (int)cy, (int)r, (int)r, pal->border, 0, 1);
    for (int i = 0; i < node->child_count; i++) {
        fastchart_pack_node *ch = node->children[i];
        pack_draw(t, ch, cx + ch->x * scale, cy + ch->y * scale, scale,
                  pal, font, size, leaf_idx);
    }
}

int fastchart_circlepack_render_to_target(fastchart_circlepack_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (!self->root) {
        zend_throw_error(NULL,
            "FastChart\\CirclePacking::draw() requires setHierarchy()");
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

    /* After empty-leaf pruning at build time a hierarchy can collapse to
     * a valueless root with no children. Treat that as no data rather
     * than drawing a lone placeholder circle. */
    if (self->root->child_count == 0 && self->root->value <= 0.0) {
        zend_throw_error(NULL,
            "FastChart\\CirclePacking::draw() requires a hierarchy with at "
            "least one positive leaf value");
        return -1;
    }

    pack_layout(self->root);
    if (self->root->r <= 0.0) return 0;

    int plot_y0 = top_pad + 8, plot_y1 = H - 12;
    int plot_x0 = 12, plot_x1 = W - 12;
    double avail = (plot_x1 - plot_x0) < (plot_y1 - plot_y0)
        ? (plot_x1 - plot_x0) : (plot_y1 - plot_y0);
    if (avail < 20.0) return 0;   /* canvas too small to draw into */
    double scale = (avail / 2.0 * 0.97) / self->root->r;
    double cx = (plot_x0 + plot_x1) / 2.0;
    double cy = (plot_y0 + plot_y1) / 2.0;

    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);
    int leaf_idx = 0;
    pack_draw(t, self->root, cx, cy, scale, &pal, font, size, &leaf_idx);

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
