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

int fastchart_surface_render_to_image(fastchart_surface_obj *self, gdImagePtr im)
{
    fastchart_target_t t;
    fastchart_target_from_gd(&t, im, self->dpi);
    if (!self->grid.cells || self->grid.rows == 0 || self->grid.cols == 0) {
        zend_throw_error(NULL,
            "FastChart\\SurfaceChart::draw() requires setGrid() with non-empty data");
        return -1;
    }
    double *grid = self->grid.cells;
    int rows = self->grid.rows;
    int cols = self->grid.cols;

    double vmin = 0, vmax = 0;
    int seen = 0;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double v = grid[i * cols + j];
            if (isnan(v)) continue;
            if (!seen) { vmin = vmax = v; seen = 1; }
            else { if (v < vmin) vmin = v; if (v > vmax) vmax = v; }
        }
    }
    if (!seen) {
        zend_throw_error(NULL,
            "FastChart\\SurfaceChart::draw() found no numeric cells");
        return -1;
    }

    /* Per-render entry: invalidate the font cache (so a runtime
     * open_basedir narrowing between draws is honored) and stamp DPI
     * on the canvas. Must come BEFORE any palette / text work. */
    fastchart_begin_render((fastchart_obj *)self, &t);

    fastchart_palette pal;
    fastchart_palette_init(&t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(&t, (fastchart_obj *)self, &pal);

    int W = gdImageSX(im);
    int H = gdImageSY(im);
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, fastchart_target_color_to_gd(&t, pal.bg));

    int top = (self->title && ZSTR_LEN(self->title) > 0) ? 32 : 12;
    int margin_x = 50;
    int margin_b = 30;
    int plot_w = W - 2 * margin_x;
    int plot_h = H - top - margin_b;
    if (plot_w < 50) plot_w = 50;
    if (plot_h < 50) plot_h = 50;

    int cell_w = plot_w / cols;
    int cell_h = plot_h / rows;
    if (cell_w < 1) cell_w = 1;
    if (cell_h < 1) cell_h = 1;

    /* Draw grid cells. A 256-entry color LUT keyed on the
     * normalized cell value is allocated once instead of per-cell.
     * Two contrasting label-text colors (dark / light) are also
     * pre-allocated here so the value-label path stops calling
     * gdImageColorAllocate per cell. */
    int low = (int)self->color_ramp_low;
    int high = (int)self->color_ramp_high;
    double span = vmax - vmin;
    if (span <= 0) span = 1.0;

    int color_lut[256];
    int rgb_lut[256];
    for (int k = 0; k < 256; k++) {
        int rgb = fastchart_lerp_rgb(low, high, (double)k / 255.0);
        rgb_lut[k] = rgb;
        color_lut[k] = gdImageColorAllocate(im,
            (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
    }
    /* Label colors go through the target so the text helper receives a
     * handle; the gd-int dichotomy on label_dark/light would otherwise
     * collide with the handle-typed `color` arg on fastchart_text_draw. */
    int label_dark  = fastchart_target_color(&t, 0x22, 0x22, 0x22, 0xFF);
    int label_light = fastchart_target_color(&t, 0xEE, 0xEE, 0xEE, 0xFF);

    for (int y_idx = 0; y_idx < rows; y_idx++) {
        for (int x_idx = 0; x_idx < cols; x_idx++) {
            double v = grid[y_idx * cols + x_idx];
            if (isnan(v)) continue;
            double tv = (v - vmin) / span;
            int idx = (int)(tv * 255.0 + 0.5);
            if (idx < 0) idx = 0; else if (idx > 255) idx = 255;
            int color = color_lut[idx];
            int rgb = rgb_lut[idx];
            int x0 = margin_x + x_idx * cell_w;
            int y0 = top + y_idx * cell_h;
            int x1 = x0 + cell_w - 1;
            int y1 = y0 + cell_h - 1;
            fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
            gdImageFilledRectangle(im, x0, y0, x1, y1, color);
            if (self->edge_color >= 0) {
                gdImageRectangle(im, x0, y0, x1, y1, (int)self->edge_color);
            }
            if (self->surface_show_values) {
                const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
                if (font) {
                    const char *fmt = self->surface_value_format
                        ? ZSTR_VAL(self->surface_value_format) : "%g";
                    char buf[32];
                    snprintf(buf, sizeof(buf), fmt, v);
                    double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
                    double size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base * 0.8);
                    int tx = (x0 + x1) / 2;
                    int ty = (y0 + y1) / 2 + (int)(size * 0.35);
                    int luma = ((rgb >> 16) & 0xFF) * 299
                             + ((rgb >>  8) & 0xFF) * 587
                             + ( rgb        & 0xFF) * 114;
                    int tc = luma > 128000 ? label_dark : label_light;
                    fastchart_text_draw(&t, font, size, tc, tx, ty,
                                        FASTCHART_ALIGN_CENTER, buf, NULL, 0);
                }
            }
        }
    }

    /* Outer frame around the heatmap. */
    int frame_x1 = margin_x + cols * cell_w - 1;
    int frame_y1 = top + rows * cell_h - 1;
    if (self->border_sides & FASTCHART_BORDER_TOP)
        gdImageLine(im, margin_x, top, frame_x1, top, fastchart_target_color_to_gd(&t, pal.border));
    if (self->border_sides & FASTCHART_BORDER_BOTTOM)
        gdImageLine(im, margin_x, frame_y1, frame_x1, frame_y1, fastchart_target_color_to_gd(&t, pal.border));
    if (self->border_sides & FASTCHART_BORDER_LEFT)
        gdImageLine(im, margin_x, top, margin_x, frame_y1, fastchart_target_color_to_gd(&t, pal.border));
    if (self->border_sides & FASTCHART_BORDER_RIGHT)
        gdImageLine(im, frame_x1, top, frame_x1, frame_y1, fastchart_target_color_to_gd(&t, pal.border));

    /* Title. */
    fastchart_draw_floating_title(&t, (fastchart_obj *)self, &pal, W / 2, 24);

    fastchart_draw_text_annotations(&t, (fastchart_obj *)self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_SurfaceChart, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\SurfaceChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_surface_obj *self = Z_FASTCHART_SURFACE_OBJ_P(ZEND_THIS);
    if (fastchart_surface_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
