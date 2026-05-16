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
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

#include <math.h>

/* Match the public setter cap so accepted data renders end-to-end.
 * 1024 gdPoint = 8 KB on the stack, well within budget. The previous
 * 512 silently dropped half of an at-cap series. */
#define MAX_POLAR_POINTS FASTCHART_MAX_POLAR_POINTS

int fastchart_polar_render_to_target(fastchart_polar_obj *self, fastchart_target_t *t)
{
    if (self->n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\PolarChart::draw() requires setSeries() with non-empty data");
        return -1;
    }
    fastchart_polar_series *series = self->series;
    int n_series = self->n_series;

    double rmax = 0;
    for (int s = 0; s < n_series; s++) {
        for (int i = 0; i < series[s].len; i++) {
            double r = series[s].radii[i];
            if (r > rmax) rmax = r;
        }
    }
    if (self->polar_max_radius > 0) rmax = self->polar_max_radius;
    if (rmax <= 0) rmax = 1.0;

    /* Per-render entry: invalidate the font cache (so a runtime
     * open_basedir narrowing between draws is honored) and stamp DPI
     * on the canvas. Must come BEFORE any palette / text work. */
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    int top = (self->title && ZSTR_LEN(self->title) > 0) ? 32 : 8;
    int cx = W / 2;
    int cy = (top + H) / 2;
    int radius = (W < H ? W : H) / 2 - 40;
    if (radius < 40) radius = 40;

    /* Concentric grid + radial spokes every 30°. */
    const int rings = 4;
    for (int r = 1; r <= rings; r++) {
        int rr = (int)((double)radius * (double)r / (double)rings);
        fastchart_target_ellipse(t, cx, cy, rr, rr, pal.grid, 0, 1);
    }
    for (int a = 0; a < 360; a += 30) {
        double rad = a * M_PI / 180.0;
        int tx = cx + (int)(radius * cos(rad));
        int ty = cy - (int)(radius * sin(rad));
        fastchart_target_line(t, cx, cy, tx, ty, pal.grid, 1, FASTCHART_DASH_SOLID);
    }

    int legend_colors[FASTCHART_MAX_POLAR_SERIES];
    const char *legend_labels[FASTCHART_MAX_POLAR_SERIES];
    int legend_count = 0;

    for (int s = 0; s < n_series; s++) {
        int color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
        if (series[s].color_rgb >= 0) {
            color = fastchart_target_color_rgb(t, series[s].color_rgb);
        }

        int upto = series[s].len < MAX_POLAR_POINTS ? series[s].len : MAX_POLAR_POINTS;

        /* STYLE_ROSE: each (angle, radius) becomes an angular wedge
         * extending from the centre out to `radius * r/rmax`. The
         * angular width is the gap to the next point's angle (or
         * 360/n if a single series is uniformly spaced). Wedges are
         * filled with the series colour and outlined in the border
         * colour for visual separation. The legacy line/area style
         * stays as the default branch below. */
        if (self->polar_style == FASTCHART_POLAR_STYLE_ROSE) {
            if (upto < 1) continue;
            for (int i = 0; i < upto; i++) {
                double a0 = series[s].angles[i];
                double a1 = (i + 1 < upto)
                    ? series[s].angles[i + 1]
                    : a0 + 360.0 / (double)upto;
                double r = series[s].radii[i];
                if (r <= 0) continue;
                int rr_px = (int)((double)radius * r / rmax);
                /* libgd arc angles are clockwise with 0° at 3-o'clock;
                 * our angles are CCW math angles. Convert by negating
                 * and adding 360 so the renderer draws the wedge oriented
                 * the same way as the line/area branch. */
                int gd_a = (int)((360.0 - a1)) % 360;
                int gd_b = (int)((360.0 - a0)) % 360;
                if (gd_a < 0) gd_a += 360;
                if (gd_b < 0) gd_b += 360;
                if (gd_b <= gd_a) gd_b += 360;
                fastchart_target_arc(t, cx, cy, rr_px, rr_px,
                                     (double)gd_a, (double)gd_b, color, 1, 0);
                fastchart_target_arc(t, cx, cy, rr_px, rr_px,
                                     (double)gd_a, (double)gd_b, pal.border, 0, 1);
            }
            if (series[s].label && legend_count < FASTCHART_MAX_POLAR_SERIES) {
                legend_colors[legend_count] = color;
                legend_labels[legend_count] = series[s].label;
                legend_count++;
            }
            continue;
        }

        gdPoint raw[MAX_POLAR_POINTS];
        int n_raw = 0;
        for (int i = 0; i < upto; i++) {
            double a = series[s].angles[i];
            double r = series[s].radii[i];
            double rad = a * M_PI / 180.0;
            double rr = radius * r / rmax;
            raw[n_raw].x = cx + (int)(rr * cos(rad));
            raw[n_raw].y = cy - (int)(rr * sin(rad));
            n_raw++;
        }
        if (n_raw < 2) continue;

        /* Optional Catmull-Rom subdivision in Cartesian space.
         * For polar_filled = true the chart is a closed polygon, so
         * boundary control points wrap (last → first). For unfilled,
         * end-segments reuse their own endpoint as the missing
         * neighbour. SUBDIV=8 per segment gives a visually smooth
         * curve without flooding the polygon buffer. */
        gdPoint poly[MAX_POLAR_POINTS];
        int n_pts = 0;
        bool smooth = (self->polar_interp == FASTCHART_INTERP_SMOOTH && n_raw >= 3);
        if (smooth) {
            enum { SUBDIV = 8 };
            int segs = self->polar_filled ? n_raw : n_raw - 1;
            for (int i = 0; i < segs; i++) {
                int i1 = i;
                int i2 = (i + 1) % n_raw;
                int i0 = self->polar_filled
                    ? (i + n_raw - 1) % n_raw
                    : (i > 0 ? i - 1 : i);
                int i3 = self->polar_filled
                    ? (i + 2) % n_raw
                    : (i + 2 < n_raw ? i + 2 : n_raw - 1);
                for (int k = 0; k < SUBDIV; k++) {
                    double tt = (double)k / (double)SUBDIV;
                    double t2 = tt * tt;
                    double t3 = t2 * tt;
                    double x = 0.5 * ((2 * raw[i1].x)
                                + (-raw[i0].x + raw[i2].x) * tt
                                + (2 * raw[i0].x - 5 * raw[i1].x
                                   + 4 * raw[i2].x - raw[i3].x) * t2
                                + (-raw[i0].x + 3 * raw[i1].x
                                   - 3 * raw[i2].x + raw[i3].x) * t3);
                    double y = 0.5 * ((2 * raw[i1].y)
                                + (-raw[i0].y + raw[i2].y) * tt
                                + (2 * raw[i0].y - 5 * raw[i1].y
                                   + 4 * raw[i2].y - raw[i3].y) * t2
                                + (-raw[i0].y + 3 * raw[i1].y
                                   - 3 * raw[i2].y + raw[i3].y) * t3);
                    if (n_pts < MAX_POLAR_POINTS) {
                        poly[n_pts].x = (int)(x + 0.5);
                        poly[n_pts].y = (int)(y + 0.5);
                        n_pts++;
                    }
                }
            }
            if (!self->polar_filled && n_pts < MAX_POLAR_POINTS) {
                poly[n_pts++] = raw[n_raw - 1];
            }
        } else {
            for (int i = 0; i < n_raw; i++) poly[n_pts++] = raw[i];
        }
        if (n_pts < 2) continue;

        if (self->polar_filled && n_pts >= 3) {
            uint32_t rgba = fastchart_target_color_to_rgba(t, color);
            int rr = (rgba >> 16) & 0xFF;
            int gg = (rgba >>  8) & 0xFF;
            int bb =  rgba        & 0xFF;
            /* gd_alpha 90 → byte 255 - 90*2 = 75. */
            int alpha = fastchart_target_color(t, rr, gg, bb, 75);
            fastchart_target_polygon(t, poly, n_pts, alpha, 1, 0);
        }
        if (self->polar_filled && n_pts >= 3) {
            fastchart_target_polygon(t, poly, n_pts, color, 0, 2);
        } else {
            for (int i = 0; i < n_pts - 1; i++) {
                fastchart_target_line(t, poly[i].x, poly[i].y,
                                      poly[i+1].x, poly[i+1].y,
                                      color, 2, FASTCHART_DASH_SOLID);
            }
        }
        /* Markers anchor to the original data points, not the
         * Catmull-Rom subdivision samples. */
        for (int i = 0; i < n_raw; i++) {
            fastchart_draw_marker(t, raw[i].x, raw[i].y,
                                  FASTCHART_MARKER_CIRCLE, 5, color);
        }
        if (series[s].label && legend_count < FASTCHART_MAX_POLAR_SERIES) {
            legend_colors[legend_count] = color;
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
    }

    /* Vector overlay: each entry is drawn as a line segment from
     * (angle, radius) to (angle_to, radius_to) with a chevron at
     * the tip. Coordinates are in the same data space as the
     * series; rmax / radius_px scaling applies. */
    if (self->vectors && self->n_vectors > 0) {
        for (int i = 0; i < self->n_vectors; i++) {
            const fastchart_polar_vector *v = &self->vectors[i];
            double a0 = v->angle * M_PI / 180.0;
            double a1 = v->angle_to * M_PI / 180.0;
            double r0 = radius * v->radius    / rmax;
            double r1 = radius * v->radius_to / rmax;
            int x0 = cx + (int)(r0 * cos(a0));
            int y0 = cy - (int)(r0 * sin(a0));
            int x1 = cx + (int)(r1 * cos(a1));
            int y1 = cy - (int)(r1 * sin(a1));
            int vc = v->color_rgb >= 0
                ? fastchart_target_color_rgb(t, v->color_rgb)
                : pal.text;
            fastchart_target_line(t, x0, y0, x1, y1, vc, 2,
                                  FASTCHART_DASH_SOLID);
            /* Arrowhead: two short legs from the tip back along the
             * vector at ±25°, length = 10% of segment length capped
             * at 12 px. */
            double sx = x1 - x0, sy = y1 - y0;
            double slen = sqrt(sx * sx + sy * sy);
            if (slen > 1.0) {
                double head_len = slen * 0.18;
                if (head_len > 12.0) head_len = 12.0;
                if (head_len < 4.0)  head_len = 4.0;
                double ang = atan2(sy, sx);
                double a_l = ang + M_PI - 0.43;  /* ~25° */
                double a_r = ang + M_PI + 0.43;
                int xl = x1 + (int)(head_len * cos(a_l));
                int yl = y1 + (int)(head_len * sin(a_l));
                int xr = x1 + (int)(head_len * cos(a_r));
                int yr = y1 + (int)(head_len * sin(a_r));
                fastchart_target_line(t, x1, y1, xl, yl, vc, 2,
                                      FASTCHART_DASH_SOLID);
                fastchart_target_line(t, x1, y1, xr, yr, vc, 2,
                                      FASTCHART_DASH_SOLID);
            }
        }
    }

    /* Title. */
    fastchart_draw_floating_title(t, (fastchart_obj *)self, &pal, W / 2, 24);

    if (legend_count > 0) {
        fastchart_rect plot = { 10, top, W - 10, H - 10 };
        fastchart_draw_legend(t, (fastchart_obj *)self, &plot, &pal,
                              legend_count, legend_colors, legend_labels);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
