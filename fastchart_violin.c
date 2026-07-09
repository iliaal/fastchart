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

/* Violin plot: per group, a gaussian kernel-density estimate of the
 * sample distribution is mirrored about the group's column centre to
 * form the violin silhouette, with a median tick overlaid. Bandwidth
 * uses Silverman's rule of thumb. */

#define VIOLIN_GRID 48

static int violin_dcmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

int fastchart_violin_render_to_target(fastchart_violin_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_paint_canvas_bg(t, (fastchart_obj *)self, &pal);

    if (self->group_count <= 0) {
        zend_throw_error(NULL,
            "FastChart\\ViolinPlot::draw() requires setGroups()");
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

    /* Global value range across every group so violins align vertically. */
    double vmin = INFINITY, vmax = -INFINITY;
    int any = 0;
    for (int g = 0; g < self->group_count; g++) {
        for (int i = 0; i < self->groups[g].n; i++) {
            double v = self->groups[g].values[i];
            if (!isfinite(v)) continue;
            if (v < vmin) vmin = v;
            if (v > vmax) vmax = v;
            any = 1;
        }
    }
    if (!any) {
        zend_throw_error(NULL, "FastChart\\ViolinPlot::draw() has no finite values");
        return -1;
    }
    if (vmax - vmin < 1e-9) { vmax += 0.5; vmin -= 0.5; }

    int label_band = (int)(base_size * 1.6);
    int plot_x0 = 40, plot_x1 = W - 16;
    int plot_y0 = top_pad + 8, plot_y1 = H - 12 - label_band;
    double plot_h = plot_y1 - plot_y0;
    if (plot_x1 - plot_x0 < 10 || plot_h < 10) return 0;

    double col_w = (double)(plot_x1 - plot_x0) / self->group_count;
    double half_max = col_w / 2.0 * 0.85;

    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);

    fastchart_point_t poly[2 * VIOLIN_GRID + 2];

    for (int g = 0; g < self->group_count; g++) {
        fastchart_violin_group *grp = &self->groups[g];
        double cx = plot_x0 + col_w * (g + 0.5);
        int color = grp->color_rgb >= 0
            ? fastchart_target_color_rgb(t, grp->color_rgb)
            : pal.series[g % FASTCHART_PALETTE_SERIES_N];

        if (grp->n < 1) continue;

        /* mean / std for Silverman bandwidth. */
        double sum = 0.0;
        int n = 0;
        for (int i = 0; i < grp->n; i++) {
            if (isfinite(grp->values[i])) { sum += grp->values[i]; n++; }
        }
        if (n < 1) continue;
        double mean = sum / n;
        double var = 0.0;
        for (int i = 0; i < grp->n; i++) {
            if (isfinite(grp->values[i])) {
                double d = grp->values[i] - mean;
                var += d * d;
            }
        }
        double sd = n > 1 ? sqrt(var / (n - 1)) : 0.0;
        double h = sd > 0.0 ? 1.06 * sd * pow((double)n, -0.2) : 0.0;
        if (h < (vmax - vmin) * 0.01) h = (vmax - vmin) * 0.02;  /* floor */
        int sample_stride = n > 1024 ? (n + 1023) / 1024 : 1;

        /* Density over the global value grid. Cache each grid point's
         * pixel y so the two polygon passes below don't recompute it. */
        double dens[VIOLIN_GRID];
        double yv[VIOLIN_GRID];
        double dmax = 0.0;
        for (int j = 0; j < VIOLIN_GRID; j++) {
            double x = vmin + (vmax - vmin) * ((double)j / (VIOLIN_GRID - 1));
            yv[j] = plot_y1 - (x - vmin) / (vmax - vmin) * plot_h;
            double s = 0.0;
            int sampled = 0;
            for (int i = 0; i < grp->n; i += sample_stride) {
                if (!isfinite(grp->values[i])) continue;
                double u = (x - grp->values[i]) / h;
                s += exp(-0.5 * u * u);
                sampled++;
            }
            if (sampled == 0) sampled = 1;
            dens[j] = s / (sampled * h * 2.5066282746310002);  /* sqrt(2pi) */
            if (dens[j] > dmax) dmax = dens[j];
        }
        if (dmax <= 0.0) dmax = 1.0;

        int np = 0;
        for (int j = 0; j < VIOLIN_GRID; j++) {
            double w = dens[j] / dmax * half_max;
            poly[np].x = (int)(cx + w);
            poly[np].y = (int)yv[j];
            np++;
        }
        for (int j = VIOLIN_GRID - 1; j >= 0; j--) {
            double w = dens[j] / dmax * half_max;
            poly[np].x = (int)(cx - w);
            poly[np].y = (int)yv[j];
            np++;
        }
        fastchart_target_polygon(t, poly, np, color, 1, 0);
        fastchart_target_polygon(t, poly, np, pal.border, 0, 1);

        /* Median tick. */
        double *sorted = emalloc(sizeof(double) * grp->n);
        int sn = 0;
        for (int i = 0; i < grp->n; i++) {
            if (isfinite(grp->values[i])) sorted[sn++] = grp->values[i];
        }
        if (sn > 0) {
            qsort(sorted, sn, sizeof(double), violin_dcmp);
            double med = sn % 2 ? sorted[sn / 2]
                                : (sorted[sn / 2 - 1] + sorted[sn / 2]) / 2.0;
            double my = plot_y1 - (med - vmin) / (vmax - vmin) * plot_h;
            fastchart_target_line(t, (int)(cx - half_max * 0.5), (int)my,
                                  (int)(cx + half_max * 0.5), (int)my,
                                  pal.text, 2, FASTCHART_DASH_SOLID);
        }
        efree(sorted);

        if (font && grp->label) {
            fastchart_text_draw(t, font, size, pal.text,
                                (int)cx, plot_y1 + (int)(size * 1.1),
                                FASTCHART_ALIGN_CENTER, grp->label, NULL, 0);
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
