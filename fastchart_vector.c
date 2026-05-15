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

/* Vector field: each datum draws an arrow at (x,y) pointing in
 * (dx,dy). Arrow length is proportional to vector magnitude
 * relative to mag_max, capped at one grid cell so dense fields
 * don't overlap. */
int fastchart_vector_render_to_target(fastchart_vector_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (self->vector_count <= 0) {
        zend_throw_error(NULL,
            "FastChart\\VectorChart::draw() requires setVectors() with at least one entry");
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

    /* Data bounds for (x,y) anchor. */
    double xmin = self->vectors[0].x, xmax = self->vectors[0].x;
    double ymin = self->vectors[0].y, ymax = self->vectors[0].y;
    for (int i = 1; i < self->vector_count; i++) {
        if (self->vectors[i].x < xmin) xmin = self->vectors[i].x;
        if (self->vectors[i].x > xmax) xmax = self->vectors[i].x;
        if (self->vectors[i].y < ymin) ymin = self->vectors[i].y;
        if (self->vectors[i].y > ymax) ymax = self->vectors[i].y;
    }
    if (xmax <= xmin) xmax = xmin + 1.0;
    if (ymax <= ymin) ymax = ymin + 1.0;
    double dxr = xmax - xmin;
    double dyr = ymax - ymin;

    fastchart_value_range xr, yr;
    fastchart_value_range_compute(xmin, xmax, 5, &xr);
    fastchart_value_range_compute(ymin, ymax, 5, &yr);

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, t, 1, 1, NULL, 0, &plot);

    /* Plot bg + frame + ticks (basic). */
    fastchart_target_rect(t, plot.x0, plot.y0,
                          plot.x1 - plot.x0 + 1, plot.y1 - plot.y0 + 1,
                          pal.plot_bg, 1, 0);
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);

    /* Light grid. */
    for (int i = 0; i < yr.n_ticks; i++) {
        double frac = (yr.ticks[i] - yr.min) / (yr.max - yr.min);
        int y = plot.y1 - (int)(frac * (plot.y1 - plot.y0));
        fastchart_target_line(t, plot.x0, y, plot.x1, y,
                              pal.grid, 1, FASTCHART_DASH_SOLID);
        if (font) {
            char buf[24];
            snprintf(buf, sizeof(buf), "%g", yr.ticks[i]);
            fastchart_text_draw(t, font, size, pal.text,
                                plot.x0 - 6, y + (int)(size * 0.4),
                                FASTCHART_ALIGN_RIGHT, buf, NULL, 0);
        }
    }
    for (int i = 0; i < xr.n_ticks; i++) {
        double frac = (xr.ticks[i] - xr.min) / (xr.max - xr.min);
        int x = plot.x0 + (int)(frac * (plot.x1 - plot.x0));
        fastchart_target_line(t, x, plot.y0, x, plot.y1,
                              pal.grid, 1, FASTCHART_DASH_SOLID);
        if (font) {
            char buf[24];
            snprintf(buf, sizeof(buf), "%g", xr.ticks[i]);
            fastchart_text_draw(t, font, size, pal.text,
                                x, plot.y1 + (int)(size * 1.4),
                                FASTCHART_ALIGN_CENTER, buf, NULL, 0);
        }
    }
    fastchart_target_rect(t, plot.x0, plot.y0,
                          plot.x1 - plot.x0 + 1, plot.y1 - plot.y0 + 1,
                          pal.border, 0, 1);

    /* Max arrow length: cap at half the smaller plot-axis tick spacing
     * so dense fields don't overlap. */
    double tick_dx = (plot.x1 - plot.x0) / (double)(xr.n_ticks > 1 ? xr.n_ticks - 1 : 1);
    double tick_dy = (plot.y1 - plot.y0) / (double)(yr.n_ticks > 1 ? yr.n_ticks - 1 : 1);
    double cap_px = (tick_dx < tick_dy ? tick_dx : tick_dy) * 0.9;
    if (cap_px < 8) cap_px = 8;
    double mag_max = self->mag_max > 0 ? self->mag_max : 1.0;

    /* Ramp colors if both endpoints set. */
    int have_ramp = (self->color_low_rgb >= 0 && self->color_high_rgb >= 0);
    int lr = 0, lg = 0, lb = 0, hr = 0, hg = 0, hb = 0;
    if (have_ramp) {
        lr = (self->color_low_rgb  >> 16) & 0xFF;
        lg = (self->color_low_rgb  >>  8) & 0xFF;
        lb = (self->color_low_rgb       ) & 0xFF;
        hr = (self->color_high_rgb >> 16) & 0xFF;
        hg = (self->color_high_rgb >>  8) & 0xFF;
        hb = (self->color_high_rgb      ) & 0xFF;
    }
    double mag_range = self->mag_max - self->mag_min;
    if (mag_range <= 0) mag_range = 1.0;

    /* Render every arrow. */
    for (int i = 0; i < self->vector_count; i++) {
        double mag = sqrt(self->vectors[i].dx * self->vectors[i].dx
                          + self->vectors[i].dy * self->vectors[i].dy);
        double scale = mag_max > 0 ? mag / mag_max : 0.0;
        double len = scale * cap_px;
        if (len < 1.5) continue;
        double axu = (xmax > xmin)
            ? (self->vectors[i].x - xmin) / dxr : 0.5;
        double ayu = (ymax > ymin)
            ? (self->vectors[i].y - ymin) / dyr : 0.5;
        int ax = plot.x0 + (int)(axu * (plot.x1 - plot.x0));
        int ay = plot.y1 - (int)(ayu * (plot.y1 - plot.y0));
        double ang = atan2(self->vectors[i].dy, self->vectors[i].dx);
        int tx = ax + (int)(cos(ang) * len);
        int ty = ay - (int)(sin(ang) * len);  /* screen-Y is inverted */
        int color;
        if (have_ramp) {
            double frac = (mag - self->mag_min) / mag_range;
            if (frac < 0) frac = 0;
            if (frac > 1) frac = 1;
            int rr = lr + (int)((hr - lr) * frac);
            int gg = lg + (int)((hg - lg) * frac);
            int bb = lb + (int)((hb - lb) * frac);
            color = fastchart_target_color_rgb(
                t, (rr << 16) | (gg << 8) | bb);
        } else if (self->vectors[i].color_rgb >= 0) {
            color = fastchart_target_color_rgb(t, self->vectors[i].color_rgb);
        } else {
            color = pal.series[0];
        }
        fastchart_target_line(t, ax, ay, tx, ty, color, 2, FASTCHART_DASH_SOLID);
        /* Arrowhead: small triangle at the tip. */
        double head_len = len * 0.3;
        if (head_len < 4) head_len = 4;
        double head_ang = 0.4;     /* ~23 degrees */
        int hx1 = tx - (int)(cos(ang - head_ang) * head_len);
        int hy1 = ty + (int)(sin(ang - head_ang) * head_len);
        int hx2 = tx - (int)(cos(ang + head_ang) * head_len);
        int hy2 = ty + (int)(sin(ang + head_ang) * head_len);
        fastchart_point_t tri[3] = {
            { tx, ty },
            { hx1, hy1 },
            { hx2, hy2 },
        };
        fastchart_target_polygon(t, tri, 3, color, 1, 0);
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
