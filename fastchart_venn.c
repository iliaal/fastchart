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

/* Venn diagram for 2 or 3 sets. Circle areas are proportional to set
 * size; pairwise centre distances are solved numerically so each lens
 * area matches the requested intersection. Circles are filled with a
 * translucent colour so overlaps blend visually. The triple-overlap
 * region (3 sets) emerges from the pairwise geometry rather than being
 * independently fitted — exact 3-set area proportionality has no general
 * solution, which is why this is deliberately capped at 3 sets. */

static double venn_clampd(double v, double lo, double hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* Area of the lens where two circles (radii r, R, centre distance d)
 * overlap. */
static double venn_lens_area(double r, double R, double d)
{
    if (d >= r + R) return 0.0;
    double m = r < R ? r : R;
    if (d <= fabs(r - R)) return M_PI * m * m;
    double d1 = (d * d - R * R + r * r) / (2.0 * d);
    double d2 = d - d1;
    double a = r * r * acos(venn_clampd(d1 / r, -1.0, 1.0))
             - d1 * sqrt(fmax(0.0, r * r - d1 * d1))
             + R * R * acos(venn_clampd(d2 / R, -1.0, 1.0))
             - d2 * sqrt(fmax(0.0, R * R - d2 * d2));
    return a;
}

/* Centre distance that makes the lens area equal `target`. Lens area is
 * monotonically decreasing in d, so bisect. */
static double venn_solve_distance(double r, double R, double target)
{
    double amax = M_PI * (r < R ? r : R) * (r < R ? r : R);
    if (target <= 0.0) return r + R;
    if (target >= amax) return fabs(r - R);
    double lo = fabs(r - R), hi = r + R;
    for (int it = 0; it < 60; it++) {
        double mid = (lo + hi) / 2.0;
        if (venn_lens_area(r, R, mid) > target) lo = mid;
        else hi = mid;
    }
    return (lo + hi) / 2.0;
}

static double venn_pair_size(fastchart_venn_obj *self, int i, int j)
{
    for (int k = 0; k < self->inter_count; k++) {
        if ((self->inters[k].a == i && self->inters[k].b == j) ||
            (self->inters[k].a == j && self->inters[k].b == i)) {
            return self->inters[k].size;
        }
    }
    return 0.0;
}

int fastchart_venn_render_to_target(fastchart_venn_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (self->set_count < 2) {
        zend_throw_error(NULL,
            "FastChart\\VennDiagram::draw() requires setSets() with 2 or 3 sets");
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

    int n = self->set_count;
    double r[3] = {0, 0, 0};
    double cx[3] = {0, 0, 0}, cy[3] = {0, 0, 0};
    for (int i = 0; i < n; i++) {
        double s = self->sets[i].size > 0.0 ? self->sets[i].size : 1.0;
        r[i] = sqrt(s / M_PI);
    }

    /* Lay out centres in abstract units. */
    double d01 = venn_solve_distance(r[0], r[1], venn_pair_size(self, 0, 1));
    cx[0] = 0.0; cy[0] = 0.0;
    cx[1] = d01; cy[1] = 0.0;
    if (n == 3) {
        double d02 = venn_solve_distance(r[0], r[2], venn_pair_size(self, 0, 2));
        double d12 = venn_solve_distance(r[1], r[2], venn_pair_size(self, 1, 2));
        if (d01 > 1e-6) {
            double x = (d01 * d01 + d02 * d02 - d12 * d12) / (2.0 * d01);
            double y2 = d02 * d02 - x * x;
            cx[2] = x;
            cy[2] = sqrt(fmax(0.0, y2));
        } else {
            cx[2] = 0.0;
            cy[2] = r[0] + r[2];
        }
    }

    /* Fit the bounding box of all circles to the plot rect. */
    double minx = INFINITY, maxx = -INFINITY, miny = INFINITY, maxy = -INFINITY;
    for (int i = 0; i < n; i++) {
        if (cx[i] - r[i] < minx) minx = cx[i] - r[i];
        if (cx[i] + r[i] > maxx) maxx = cx[i] + r[i];
        if (cy[i] - r[i] < miny) miny = cy[i] - r[i];
        if (cy[i] + r[i] > maxy) maxy = cy[i] + r[i];
    }
    int plot_x0 = 16, plot_x1 = W - 16;
    int plot_y0 = top_pad + 8, plot_y1 = H - 16;
    double bw = maxx - minx, bh = maxy - miny;
    if (bw < 1e-6) bw = 1.0;
    if (bh < 1e-6) bh = 1.0;
    double sx = (plot_x1 - plot_x0) * 0.9 / bw;
    double sy = (plot_y1 - plot_y0) * 0.9 / bh;
    double scale = sx < sy ? sx : sy;
    double off_x = (plot_x0 + plot_x1) / 2.0 - (minx + maxx) / 2.0 * scale;
    double off_y = (plot_y0 + plot_y1) / 2.0 - (miny + maxy) / 2.0 * scale;

    double mid_x = 0.0, mid_y = 0.0;
    for (int i = 0; i < n; i++) { mid_x += cx[i]; mid_y += cy[i]; }
    mid_x /= n; mid_y /= n;

    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);

    for (int i = 0; i < n; i++) {
        int rgb;
        if (self->sets[i].color_rgb >= 0) {
            rgb = self->sets[i].color_rgb;
        } else {
            uint32_t rgba = fastchart_target_color_to_rgba(t, pal.series[i % FASTCHART_PALETTE_SERIES_N]);
            rgb = (int)(rgba & 0xFFFFFF);
        }
        int cr = (rgb >> 16) & 0xFF, cg = (rgb >> 8) & 0xFF, cb = rgb & 0xFF;
        int fill = fastchart_target_color(t, cr, cg, cb, 115);
        int stroke = fastchart_target_color(t, cr, cg, cb, 255);

        int px = (int)(cx[i] * scale + off_x);
        int py = (int)(cy[i] * scale + off_y);
        int rr = (int)(r[i] * scale);
        fastchart_target_ellipse(t, px, py, rr, rr, fill, 1, 0);
        fastchart_target_ellipse(t, px, py, rr, rr, stroke, 0, 2);

        if (font && self->sets[i].label) {
            /* Push the label outward from the group centroid. */
            double dxn = cx[i] - mid_x, dyn = cy[i] - mid_y;
            double len = sqrt(dxn * dxn + dyn * dyn);
            int lx = px, ly = py;
            if (len > 1e-6) {
                lx = (int)(px + dxn / len * rr * 0.55);
                ly = (int)(py + dyn / len * rr * 0.55);
            }
            fastchart_text_draw(t, font, size, pal.text,
                                lx, ly + (int)(size * 0.35),
                                FASTCHART_ALIGN_CENTER,
                                self->sets[i].label, NULL, 0);
        }
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
