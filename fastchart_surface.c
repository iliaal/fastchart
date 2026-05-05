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

int fastchart_surface_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    HashTable *grid = Z_TYPE(self->data) == IS_ARRAY ? Z_ARRVAL(self->data) : NULL;
    if (!grid || zend_hash_num_elements(grid) == 0) {
        zend_throw_error(NULL,
            "FastChart\\SurfaceChart::draw() requires setGrid() with non-empty data");
        return -1;
    }

    int rows = (int)zend_hash_num_elements(grid);
    int cols = 0;
    /* Determine column count from the longest row, and find min/max
     * across all numeric cells. */
    double vmin = 0, vmax = 0;
    int seen = 0;
    zval *row;
    ZEND_HASH_FOREACH_VAL(grid, row) {
        if (Z_TYPE_P(row) != IS_ARRAY) continue;
        int rlen = (int)zend_hash_num_elements(Z_ARRVAL_P(row));
        if (rlen > cols) cols = rlen;
        zval *cell;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(row), cell) {
            double v;
            if (fastchart_zval_to_double(cell, &v) != 0) continue;
            if (!seen) { vmin = vmax = v; seen = 1; }
            else { if (v < vmin) vmin = v; if (v > vmax) vmax = v; }
        } ZEND_HASH_FOREACH_END();
    } ZEND_HASH_FOREACH_END();

    if (!seen || rows == 0 || cols == 0) {
        zend_throw_error(NULL,
            "FastChart\\SurfaceChart::draw() found no numeric cells");
        return -1;
    }

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    int W = gdImageSX(im);
    int H = gdImageSY(im);
    gdImageFilledRectangle(im, 0, 0, W - 1, H - 1, pal.bg);

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

    /* Draw grid cells. */
    int low = (int)self->surface_low;
    int high = (int)self->surface_high;
    double span = vmax - vmin;
    if (span <= 0) span = 1.0;

    int y_idx = 0;
    ZEND_HASH_FOREACH_VAL(grid, row) {
        if (Z_TYPE_P(row) != IS_ARRAY) { y_idx++; continue; }
        int x_idx = 0;
        zval *cell;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(row), cell) {
            double v;
            if (fastchart_zval_to_double(cell, &v) == 0) {
                double t = (v - vmin) / span;
                int rgb = fastchart_lerp_rgb(low, high, t);
                int color = gdImageColorAllocate(im,
                    (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
                int x0 = margin_x + x_idx * cell_w;
                int y0 = top + y_idx * cell_h;
                int x1 = x0 + cell_w - 1;
                int y1 = y0 + cell_h - 1;
                fastchart_shadow_filled_rectangle(im, self, x0, y0, x1, y1);
                gdImageFilledRectangle(im, x0, y0, x1, y1, color);
                if (self->edge_color >= 0) {
                    gdImageRectangle(im, x0, y0, x1, y1, (int)self->edge_color);
                }
                if (self->surface_show_values) {
                    const char *font = fastchart_resolve_font(self, "label");
                    if (font) {
                        const char *fmt = self->surface_value_format
                            ? ZSTR_VAL(self->surface_value_format) : "%g";
                        char buf[32];
                        snprintf(buf, sizeof(buf), fmt, v);
                        double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
                        double size = fastchart_resolve_font_size(self, "label", base * 0.8);
                        int tx = (x0 + x1) / 2;
                        int ty = (y0 + y1) / 2 + (int)(size * 0.35);
                        /* Pick contrasting text color. */
                        int luma = ((rgb >> 16) & 0xFF) * 299
                                 + ((rgb >>  8) & 0xFF) * 587
                                 + ( rgb        & 0xFF) * 114;
                        int tc = luma > 128000
                            ? gdImageColorAllocate(im, 0x22, 0x22, 0x22)
                            : gdImageColorAllocate(im, 0xEE, 0xEE, 0xEE);
                        fastchart_text_draw(im, font, size, tc, tx, ty,
                                            FASTCHART_ALIGN_CENTER, buf, NULL, 0);
                    }
                }
            }
            x_idx++;
            if (x_idx >= cols) break;
        } ZEND_HASH_FOREACH_END();
        y_idx++;
        if (y_idx >= rows) break;
    } ZEND_HASH_FOREACH_END();

    /* Outer frame around the heatmap. */
    int frame_x1 = margin_x + cols * cell_w - 1;
    int frame_y1 = top + rows * cell_h - 1;
    if (self->border_sides & FASTCHART_BORDER_TOP)
        gdImageLine(im, margin_x, top, frame_x1, top, pal.border);
    if (self->border_sides & FASTCHART_BORDER_BOTTOM)
        gdImageLine(im, margin_x, frame_y1, frame_x1, frame_y1, pal.border);
    if (self->border_sides & FASTCHART_BORDER_LEFT)
        gdImageLine(im, margin_x, top, margin_x, frame_y1, pal.border);
    if (self->border_sides & FASTCHART_BORDER_RIGHT)
        gdImageLine(im, frame_x1, top, frame_x1, frame_y1, pal.border);

    /* Title. */
    const char *font = fastchart_resolve_font(self, "title");
    if (self->title && ZSTR_LEN(self->title) > 0 && font && !self->thumbnail_mode) {
        double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
        double size = fastchart_resolve_font_size(self, "title", base * 1.4);
        int color = self->title_color >= 0 ? (int)self->title_color : pal.text;
        fastchart_shadow_text(im, self, font, size, W / 2, 24, 0.0, ZSTR_VAL(self->title));
        fastchart_text_draw(im, font, size, color, W / 2, 24,
                            FASTCHART_ALIGN_CENTER, ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(im, self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_SurfaceChart, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) RETURN_THROWS();
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_surface_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
