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
#include "fastchart_axis.h"
#include "fastchart_text.h"

/* Default heatmap ramp: cool blue → warm red. Same as the contour
 * default; the heatmap is essentially a contour with discrete
 * cell-color bins instead of interpolated isolines. */
#define HM_DEFAULT_LOW   0x2E5CB8
#define HM_DEFAULT_HIGH  0xE34A6F

static int interp_color(int rgb_lo, int rgb_hi, double t)
{
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    int r0 = (rgb_lo >> 16) & 0xFF, g0 = (rgb_lo >> 8) & 0xFF, b0 = rgb_lo & 0xFF;
    int r1 = (rgb_hi >> 16) & 0xFF, g1 = (rgb_hi >> 8) & 0xFF, b1 = rgb_hi & 0xFF;
    int r = (int)(r0 + (r1 - r0) * t + 0.5);
    int g = (int)(g0 + (g1 - g0) * t + 0.5);
    int b = (int)(b0 + (b1 - b0) * t + 0.5);
    return (r << 16) | (g << 8) | b;
}

int fastchart_heatmap_render_to_image(fastchart_heatmap_obj *self, gdImagePtr im)
{
    if (!self->grid.cells || self->grid.rows <= 0 || self->grid.cols <= 0) {
        zend_throw_error(NULL,
            "FastChart\\Heatmap::draw() requires setGrid() with a non-empty 2D array");
        return -1;
    }

    fastchart_begin_render((fastchart_obj *)self, im);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, (fastchart_obj *)self, &pal);

    int W = gdImageSX(im);
    int H = gdImageSY(im);
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal.bg);

    /* Title reservation (optional). */
    int top_pad = 12;
    int title_h = 0;
    const char *title_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_TITLE);
    double base_size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double title_size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_TITLE, base_size * 1.4);
    if (self->title && ZSTR_LEN(self->title) > 0 && title_font) {
        if (fastchart_text_measure(im, title_font, title_size, ZSTR_VAL(self->title),
                                   NULL, &title_h, NULL, 0) == 0) {
            top_pad += title_h + 10;
        }
    }

    int x0 = 12, y0 = top_pad, x1 = W - 13, y1 = H - 13;
    if (self->has_plot_rect) {
        x0 = (int)self->plot_x0; y0 = (int)self->plot_y0;
        x1 = (int)self->plot_x1; y1 = (int)self->plot_y1;
    }

    int rows = self->grid.rows;
    int cols = self->grid.cols;
    int plot_w = x1 - x0 + 1;
    int plot_h = y1 - y0 + 1;
    if (plot_w < cols || plot_h < rows) {
        zend_throw_error(NULL,
            "FastChart\\Heatmap::draw() plot area too small for the grid (%dx%d cells need >= %dx%d px)",
            cols, rows, cols, rows);
        return -1;
    }

    /* Find min/max so we can normalise into [0, 1] for the ramp. */
    double v_min = INFINITY, v_max = -INFINITY;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            double v = self->grid.cells[r * cols + c];
            if (!isfinite(v)) continue;
            if (v < v_min) v_min = v;
            if (v > v_max) v_max = v;
        }
    }
    if (!isfinite(v_min) || !isfinite(v_max) || v_max <= v_min) {
        zend_throw_error(NULL,
            "FastChart\\Heatmap::draw() requires the grid to contain at least two distinct finite values");
        return -1;
    }

    int rgb_lo = self->color_low_rgb  >= 0 ? self->color_low_rgb  : HM_DEFAULT_LOW;
    int rgb_hi = self->color_high_rgb >= 0 ? self->color_high_rgb : HM_DEFAULT_HIGH;

    fastchart_color_cache cache;
    fastchart_color_cache_init(&cache);

    /* Cells: integer pixel coords distributed across the plot area
     * with no rounding gap between cells. The "previous edge" trick
     * (use floor(i*w/cols) for both left and "right of previous")
     * keeps adjacent cells touching even when plot_w doesn't divide
     * evenly by cols. */
    const char *label_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double label_size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base_size * 0.85);

    for (int r = 0; r < rows; r++) {
        int cell_y0 = y0 + (int)((double)r * (double)plot_h / (double)rows);
        int cell_y1 = y0 + (int)((double)(r + 1) * (double)plot_h / (double)rows) - 1;
        if (cell_y1 < cell_y0) cell_y1 = cell_y0;
        for (int c = 0; c < cols; c++) {
            int cell_x0 = x0 + (int)((double)c * (double)plot_w / (double)cols);
            int cell_x1 = x0 + (int)((double)(c + 1) * (double)plot_w / (double)cols) - 1;
            if (cell_x1 < cell_x0) cell_x1 = cell_x0;

            double v = self->grid.cells[r * cols + c];
            int color;
            if (!isfinite(v)) {
                color = pal.bg;  /* missing data leaves the canvas bg showing */
            } else {
                double t = (v - v_min) / (v_max - v_min);
                int rgb = interp_color(rgb_lo, rgb_hi, t);
                color = fastchart_color_cache_get(&cache, im, rgb);
            }
            gdImageFilledRectangle(im, cell_x0, cell_y0, cell_x1, cell_y1, color);
            gdImageRectangle(im, cell_x0, cell_y0, cell_x1, cell_y1, pal.border);

            fastchart_obj *base = (fastchart_obj *)self;
            if (base->show_values && label_font && isfinite(v)) {
                int cell_w = cell_x1 - cell_x0 + 1;
                int cell_h = cell_y1 - cell_y0 + 1;
                if (cell_w < (int)(label_size * 2.5) || cell_h < (int)(label_size * 1.6)) continue;

                char buf[32];
                if (base->value_format) {
                    snprintf(buf, sizeof(buf),
                             ZSTR_VAL(base->value_format), v);
                } else {
                    snprintf(buf, sizeof(buf), "%g", v);
                }
                int tw = 0, th = 0;
                if (fastchart_text_measure(im, label_font, label_size, buf,
                                           &tw, &th, NULL, 0) != 0) continue;
                if (tw > cell_w - 4) continue;

                /* Pick contrast against the cell colour. */
                int rr = gdImageRed(im, color);
                int gg = gdImageGreen(im, color);
                int bb = gdImageBlue(im, color);
                int luma = (299 * rr + 587 * gg + 114 * bb) / 1000;
                int label_color = luma > 145 ? gdImageColorAllocate(im, 0, 0, 0)
                                             : gdImageColorAllocate(im, 255, 255, 255);

                int cx = (cell_x0 + cell_x1) / 2;
                int cy = (cell_y0 + cell_y1) / 2 + th / 2;
                fastchart_text_draw(im, label_font, label_size, label_color,
                                    cx, cy, FASTCHART_ALIGN_CENTER, buf, NULL, 0);
            }
        }
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(im, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(im, (fastchart_obj *)self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_Heatmap, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();
    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\Heatmap::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_heatmap_obj *self = Z_FASTCHART_HEATMAP_OBJ_P(ZEND_THIS);
    if (fastchart_heatmap_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
