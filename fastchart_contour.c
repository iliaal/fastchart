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
#include "fastchart_effects.h"

#include <math.h>

/* Marching-squares isoline drawing through a 2D scalar grid. For
 * each cell (square between four neighboring grid points) we compute
 * a 4-bit case index (one bit per corner above the threshold) and
 * draw 0, 1, or 2 line segments connecting the threshold crossings
 * along the cell's edges.
 *
 * The grid arrives as a HashTable<HashTable<zval>> from PHP. We
 * materialize once into a flat double[rows*cols] (NAN for missing /
 * non-numeric cells) so the marching-squares loop can index by
 * arithmetic instead of nested hash lookups, which were costing two
 * zend_hash_index_find calls per corner per cell per level. */

static void cell_to_pixel(int col, int row, double col_f, double row_f,
                          int x0, int y0, double cell_w, double cell_h,
                          int *px, int *py)
{
    double cx = (double)col + col_f;
    double cy = (double)row + row_f;
    *px = x0 + (int)(cx * cell_w + 0.5);
    *py = y0 + (int)(cy * cell_h + 0.5);
}

/* Linear interpolation parameter where value crosses level.
 * Returned value is clamped to [0, 1] so even pathological inputs
 * (a == b == level, NaN/Inf in the cell from earlier non-finite
 * arithmetic) can't propagate into cell_to_pixel as a non-finite
 * fractional coord and round to a wild int via cast. */
static double t_cross(double a, double b, double level)
{
    double d = b - a;
    if (d == 0 || !isfinite(d)) return 0.5;
    double t = (level - a) / d;
    if (!isfinite(t)) return 0.5;
    if (t < 0.0) return 0.0;
    if (t > 1.0) return 1.0;
    return t;
}

int fastchart_contour_render_to_target(fastchart_contour_obj *self, fastchart_target_t *t)
{
    if (!self->grid.cells || self->grid.rows < 2 || self->grid.cols < 2) {
        zend_throw_error(NULL,
            "FastChart\\ContourChart::draw() requires setGrid() with at least a 2x2 grid");
        return -1;
    }
    double *grid = self->grid.cells;
    int rows = self->grid.rows;
    int cols = self->grid.cols;
#define G(i, j) grid[(i) * cols + (j)]

    double vmin = 0, vmax = 0;
    int seen = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double v = G(i, j);
            if (isnan(v)) continue;
            if (!seen) { vmin = vmax = v; seen = 1; }
            else { if (v < vmin) vmin = v; if (v > vmax) vmax = v; }
        }
    }
    if (!seen) {
        zend_throw_error(NULL,
            "FastChart\\ContourChart::draw() found no numeric values");
        return -1;
    }

    /* Levels: user-supplied via setLevels (typed) or 5 evenly spaced. */
    /* Match the public setter cap (FASTCHART_MAX_LEVELS = 32) so all
     * accepted levels render. The previous 16 silently dropped the
     * upper half of a setLevels() call at the advertised maximum. */
    double levels[FASTCHART_MAX_LEVELS];
    int n_levels = 0;
    if (self->levels && self->level_count > 0) {
        n_levels = self->level_count > FASTCHART_MAX_LEVELS
            ? FASTCHART_MAX_LEVELS
            : self->level_count;
        for (int k = 0; k < n_levels; k++) levels[k] = self->levels[k];
    }
    if (n_levels == 0) {
        n_levels = 5;
        for (int k = 0; k < 5; k++) {
            levels[k] = vmin + (vmax - vmin) * (k + 1) / (n_levels + 1);
        }
    }

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

    int top = (self->title && ZSTR_LEN(self->title) > 0) ? 32 : 12;
    int margin = 30;
    int x0 = margin;
    int y0 = top;
    int x1 = W - margin;
    int y1 = H - margin;
    if (x1 - x0 < 50) x1 = x0 + 50;
    if (y1 - y0 < 50) y1 = y0 + 50;

    double cell_w = (double)(x1 - x0) / (double)(cols - 1);
    double cell_h = (double)(y1 - y0) / (double)(rows - 1);

    /* Filled-mode background: paint each cell with the average-value color
     * before drawing isolines. A 256-entry color LUT keyed on the
     * normalized value is allocated once instead of per-cell, which
     * matters on large grids (a 100x100 grid is 10k allocations the
     * old way). */
    if (self->contour_filled) {
        int low = (int)self->color_ramp_low;
        int high = (int)self->color_ramp_high;
        double span = vmax - vmin;
        if (span <= 0) span = 1.0;

        int lut[256];
        for (int k = 0; k < 256; k++) {
            int rgb = fastchart_lerp_rgb(low, high, (double)k / 255.0);
            lut[k] = fastchart_target_color_rgb(t, rgb);
        }

        for (int i = 0; i < rows - 1; i++) {
            for (int j = 0; j < cols - 1; j++) {
                double v00 = G(i,     j);
                double v01 = G(i,     j + 1);
                double v10 = G(i + 1, j);
                double v11 = G(i + 1, j + 1);
                if (isnan(v00) || isnan(v01) || isnan(v10) || isnan(v11)) continue;
                double avg = (v00 + v01 + v10 + v11) * 0.25;
                double tv = (avg - vmin) / span;
                int idx = (int)(tv * 255.0 + 0.5);
                if (idx < 0) idx = 0; else if (idx > 255) idx = 255;
                int color = lut[idx];
                int px0 = x0 + (int)(j * cell_w);
                int py0 = y0 + (int)(i * cell_h);
                int px1 = x0 + (int)((j + 1) * cell_w);
                int py1 = y0 + (int)((i + 1) * cell_h);
                fastchart_target_rect(t, px0, py0,
                                      px1 - px0 + 1, py1 - py0 + 1,
                                      color, 1, 0);
            }
        }
    }

    int isoline_color = self->edge_color >= 0
        ? fastchart_target_color_rgb(t, (int)self->edge_color)
        : pal.axis;

    /* Marching squares per level. */
    for (int k = 0; k < n_levels; k++) {
        double L = levels[k];
        for (int i = 0; i < rows - 1; i++) {
            for (int j = 0; j < cols - 1; j++) {
                double v00 = G(i,     j);
                double v01 = G(i,     j + 1);
                double v10 = G(i + 1, j);
                double v11 = G(i + 1, j + 1);
                if (isnan(v00) || isnan(v01) || isnan(v10) || isnan(v11)) continue;

                int idx = 0;
                if (v00 >= L) idx |= 1;
                if (v01 >= L) idx |= 2;
                if (v11 >= L) idx |= 4;
                if (v10 >= L) idx |= 8;

                /* Crossing parameters along each cell edge:
                 *   top (j -> j+1) at row i
                 *   right (i -> i+1) at col j+1
                 *   bottom (j+1 -> j) at row i+1
                 *   left (i+1 -> i) at col j
                 *
                 * top fractional col = t_cross(v00, v01, L), row 0
                 * right: row = t_cross(v01, v11, L), col 1
                 * bottom: col = 1 - t_cross(v10, v11, L), row 1
                 * left: row = 1 - t_cross(v00, v10, L), col 0
                 */
                int p_top_x, p_top_y;
                int p_right_x, p_right_y;
                int p_bot_x, p_bot_y;
                int p_left_x, p_left_y;
                cell_to_pixel(j, i, t_cross(v00, v01, L), 0,
                              x0, y0, cell_w, cell_h, &p_top_x, &p_top_y);
                cell_to_pixel(j + 1, i, 0, t_cross(v01, v11, L),
                              x0, y0, cell_w, cell_h, &p_right_x, &p_right_y);
                cell_to_pixel(j, i + 1, t_cross(v10, v11, L), 0,
                              x0, y0, cell_w, cell_h, &p_bot_x, &p_bot_y);
                cell_to_pixel(j, i, 0, t_cross(v00, v10, L),
                              x0, y0, cell_w, cell_h, &p_left_x, &p_left_y);

                /* 16 cases. Cases 0 and 15 -> no crossing. Saddle
                 * cases (5, 10) draw two segments; resolve by averaging
                 * to the canonical pairing. */
                int color = isoline_color;
                switch (idx) {
                    case 1: case 14:
                        fastchart_target_line(t, p_left_x, p_left_y, p_top_x, p_top_y, color, 1, FASTCHART_DASH_SOLID); break;
                    case 2: case 13:
                        fastchart_target_line(t, p_top_x, p_top_y, p_right_x, p_right_y, color, 1, FASTCHART_DASH_SOLID); break;
                    case 3: case 12:
                        fastchart_target_line(t, p_left_x, p_left_y, p_right_x, p_right_y, color, 1, FASTCHART_DASH_SOLID); break;
                    case 4: case 11:
                        fastchart_target_line(t, p_right_x, p_right_y, p_bot_x, p_bot_y, color, 1, FASTCHART_DASH_SOLID); break;
                    case 6: case 9:
                        fastchart_target_line(t, p_top_x, p_top_y, p_bot_x, p_bot_y, color, 1, FASTCHART_DASH_SOLID); break;
                    case 7: case 8:
                        fastchart_target_line(t, p_left_x, p_left_y, p_bot_x, p_bot_y, color, 1, FASTCHART_DASH_SOLID); break;
                    case 5:
                        fastchart_target_line(t, p_left_x, p_left_y, p_top_x, p_top_y, color, 1, FASTCHART_DASH_SOLID);
                        fastchart_target_line(t, p_right_x, p_right_y, p_bot_x, p_bot_y, color, 1, FASTCHART_DASH_SOLID);
                        break;
                    case 10:
                        fastchart_target_line(t, p_top_x, p_top_y, p_right_x, p_right_y, color, 1, FASTCHART_DASH_SOLID);
                        fastchart_target_line(t, p_left_x, p_left_y, p_bot_x, p_bot_y, color, 1, FASTCHART_DASH_SOLID);
                        break;
                    default: break;
                }
            }
        }
    }

    /* Frame. */
    if (self->border_sides & FASTCHART_BORDER_TOP)
        fastchart_target_line(t, x0, y0, x1, y0, pal.border, 1, FASTCHART_DASH_SOLID);
    if (self->border_sides & FASTCHART_BORDER_BOTTOM)
        fastchart_target_line(t, x0, y1, x1, y1, pal.border, 1, FASTCHART_DASH_SOLID);
    if (self->border_sides & FASTCHART_BORDER_LEFT)
        fastchart_target_line(t, x0, y0, x0, y1, pal.border, 1, FASTCHART_DASH_SOLID);
    if (self->border_sides & FASTCHART_BORDER_RIGHT)
        fastchart_target_line(t, x1, y0, x1, y1, pal.border, 1, FASTCHART_DASH_SOLID);

    /* Title. */
    fastchart_draw_floating_title(t, (fastchart_obj *)self, &pal, W / 2, 24);

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
#undef G
}

/* GD-only shim. */
int fastchart_contour_render_to_image(fastchart_contour_obj *self, gdImagePtr im)
{
    fastchart_target_t t;
    fastchart_target_from_gd(&t, im, self->dpi);
    return fastchart_contour_render_to_target(self, &t);
}

ZEND_METHOD(FastChart_ContourChart, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();
    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\ContourChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_contour_obj *self = Z_FASTCHART_CONTOUR_OBJ_P(ZEND_THIS);
    if (fastchart_contour_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
