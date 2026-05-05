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
#include "fastchart_axis.h"
#include "fastchart_text.h"
#include "fastchart_effects.h"

#include <math.h>

/* Marching-squares isoline drawing through a 2D scalar grid. For
 * each cell (square between four neighboring grid points) we compute
 * a 4-bit case index (one bit per corner above the threshold) and
 * draw 0, 1, or 2 line segments connecting the threshold crossings
 * along the cell's edges. */

static double cell_value(zval *grid, int row, int col)
{
    if (Z_TYPE_P(grid) != IS_ARRAY) return NAN;
    zval *r = zend_hash_index_find(Z_ARRVAL_P(grid), row);
    if (!r || Z_TYPE_P(r) != IS_ARRAY) return NAN;
    zval *c = zend_hash_index_find(Z_ARRVAL_P(r), col);
    if (!c) return NAN;
    double d;
    if (fastchart_zval_to_double(c, &d) != 0) return NAN;
    return d;
}

static int grid_dims(zval *grid, int *rows, int *cols)
{
    if (Z_TYPE_P(grid) != IS_ARRAY) return -1;
    *rows = (int)zend_hash_num_elements(Z_ARRVAL_P(grid));
    *cols = 0;
    zval *r;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(grid), r) {
        if (Z_TYPE_P(r) != IS_ARRAY) continue;
        int n = (int)zend_hash_num_elements(Z_ARRVAL_P(r));
        if (n > *cols) *cols = n;
    } ZEND_HASH_FOREACH_END();
    return (*rows >= 2 && *cols >= 2) ? 0 : -1;
}

static void cell_to_pixel(int col, int row, double col_f, double row_f,
                          int x0, int y0, double cell_w, double cell_h,
                          int *px, int *py)
{
    double cx = (double)col + col_f;
    double cy = (double)row + row_f;
    *px = x0 + (int)(cx * cell_w + 0.5);
    *py = y0 + (int)(cy * cell_h + 0.5);
}

/* Linear interpolation parameter where value crosses level. */
static double t_cross(double a, double b, double level)
{
    double d = b - a;
    if (d == 0) return 0.5;
    return (level - a) / d;
}

int fastchart_contour_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    int rows, cols;
    if (grid_dims(&self->data, &rows, &cols) != 0) {
        zend_throw_error(NULL,
            "FastChart\\ContourChart::draw() requires setGrid() with at least a 2x2 grid");
        return -1;
    }

    /* Find global min/max. */
    double vmin = 0, vmax = 0;
    int seen = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double v = cell_value(&self->data, i, j);
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

    /* Levels: user-supplied or 5 evenly spaced between min and max. */
    double levels[16];
    int n_levels = 0;
    zval *lvls_zv = zend_hash_str_find(Z_ARRVAL(self->config),
        "contour_levels", sizeof("contour_levels") - 1);
    if (lvls_zv && Z_TYPE_P(lvls_zv) == IS_ARRAY &&
        zend_hash_num_elements(Z_ARRVAL_P(lvls_zv)) > 0) {
        zval *v;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(lvls_zv), v) {
            if (n_levels >= 16) break;
            double d;
            if (fastchart_zval_to_double(v, &d) == 0) levels[n_levels++] = d;
        } ZEND_HASH_FOREACH_END();
    }
    if (n_levels == 0) {
        n_levels = 5;
        for (int k = 0; k < 5; k++) {
            levels[k] = vmin + (vmax - vmin) * (k + 1) / (n_levels + 1);
        }
    }

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    int W = gdImageSX(im);
    int H = gdImageSY(im);
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal.bg);

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
     * before drawing isolines. */
    if (self->contour_filled) {
        int low = (int)self->contour_low;
        int high = (int)self->contour_high;
        double span = vmax - vmin;
        if (span <= 0) span = 1.0;
        for (int i = 0; i < rows - 1; i++) {
            for (int j = 0; j < cols - 1; j++) {
                double v00 = cell_value(&self->data, i,     j);
                double v01 = cell_value(&self->data, i,     j + 1);
                double v10 = cell_value(&self->data, i + 1, j);
                double v11 = cell_value(&self->data, i + 1, j + 1);
                if (isnan(v00) || isnan(v01) || isnan(v10) || isnan(v11)) continue;
                double avg = (v00 + v01 + v10 + v11) * 0.25;
                double t = (avg - vmin) / span;
                int rgb = fastchart_lerp_rgb(low, high, t);
                int color = gdImageColorAllocate(im,
                    (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
                int px0 = x0 + (int)(j * cell_w);
                int py0 = y0 + (int)(i * cell_h);
                int px1 = x0 + (int)((j + 1) * cell_w);
                int py1 = y0 + (int)((i + 1) * cell_h);
                gdImageFilledRectangle(im, px0, py0, px1, py1, color);
            }
        }
    }

    /* Marching squares per level. */
    for (int k = 0; k < n_levels; k++) {
        double L = levels[k];
        for (int i = 0; i < rows - 1; i++) {
            for (int j = 0; j < cols - 1; j++) {
                double v00 = cell_value(&self->data, i,     j);
                double v01 = cell_value(&self->data, i,     j + 1);
                double v10 = cell_value(&self->data, i + 1, j);
                double v11 = cell_value(&self->data, i + 1, j + 1);
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
                int color = self->edge_color >= 0 ? (int)self->edge_color : pal.axis;
                switch (idx) {
                    case 1: case 14:
                        gdImageLine(im, p_left_x, p_left_y, p_top_x, p_top_y, color); break;
                    case 2: case 13:
                        gdImageLine(im, p_top_x, p_top_y, p_right_x, p_right_y, color); break;
                    case 3: case 12:
                        gdImageLine(im, p_left_x, p_left_y, p_right_x, p_right_y, color); break;
                    case 4: case 11:
                        gdImageLine(im, p_right_x, p_right_y, p_bot_x, p_bot_y, color); break;
                    case 6: case 9:
                        gdImageLine(im, p_top_x, p_top_y, p_bot_x, p_bot_y, color); break;
                    case 7: case 8:
                        gdImageLine(im, p_left_x, p_left_y, p_bot_x, p_bot_y, color); break;
                    case 5:
                        gdImageLine(im, p_left_x, p_left_y, p_top_x, p_top_y, color);
                        gdImageLine(im, p_right_x, p_right_y, p_bot_x, p_bot_y, color);
                        break;
                    case 10:
                        gdImageLine(im, p_top_x, p_top_y, p_right_x, p_right_y, color);
                        gdImageLine(im, p_left_x, p_left_y, p_bot_x, p_bot_y, color);
                        break;
                    default: break;
                }
            }
        }
    }

    /* Frame. */
    if (self->border_sides & FASTCHART_BORDER_TOP)
        gdImageLine(im, x0, y0, x1, y0, pal.border);
    if (self->border_sides & FASTCHART_BORDER_BOTTOM)
        gdImageLine(im, x0, y1, x1, y1, pal.border);
    if (self->border_sides & FASTCHART_BORDER_LEFT)
        gdImageLine(im, x0, y0, x0, y1, pal.border);
    if (self->border_sides & FASTCHART_BORDER_RIGHT)
        gdImageLine(im, x1, y0, x1, y1, pal.border);

    /* Title. */
    fastchart_draw_floating_title(im, self, &pal, W / 2, 24);

    fastchart_draw_text_annotations(im, self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_ContourChart, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();
    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) RETURN_THROWS();
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_contour_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
