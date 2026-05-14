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
#include <string.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

/* Squarify (Bruls, Huijsen, van Wijk 2000): given a list of areas
 * and a target rect, lay them into rows along the rect's shorter
 * side so each cell's aspect ratio stays as close to 1 as possible.
 *
 * The routine works on a sorted-descending value list. It greedily
 * grows a "row" of cells along the shorter side; before adding a
 * new cell, it checks whether the new max aspect ratio in the row
 * gets worse and, if so, commits the current row at the rect edge
 * and starts a new row in the remaining rect.
 *
 * `rects_out` receives the laid-out pixel rectangles in the SAME
 * order as `values`; the caller paints them. */

typedef struct {
    int x0, y0, x1, y1;   /* inclusive bounds */
} treemap_rect;

/* worst aspect ratio of a row of cells with total area S in a strip
 * of length L: max(L^2 * max_v / S^2, S^2 / (L^2 * min_v)). */
static double worst_aspect(double row_sum, double row_min, double row_max,
                           double L)
{
    if (row_sum <= 0 || L <= 0) return INFINITY;
    double L2 = L * L;
    double S2 = row_sum * row_sum;
    double a = (L2 * row_max) / S2;
    double b = S2 / (L2 * row_min);
    return a > b ? a : b;
}

/* Lay a committed row of `n_row` items inside a strip along the
 * SHORTER side of the rect. Updates `*rect` to remove the consumed
 * strip. Items are written into rects_out using their original
 * indices stored in `idx_row`. */
static void place_row(const double *areas,
                      const int *idx_row, int n_row,
                      treemap_rect *rect, double total_area_remaining,
                      treemap_rect *rects_out)
{
    int w = rect->x1 - rect->x0 + 1;
    int h = rect->y1 - rect->y0 + 1;
    double row_sum = 0;
    for (int k = 0; k < n_row; k++) row_sum += areas[idx_row[k]];

    /* Strip thickness = (sum / remaining_total) * shorter_side, in
     * pixels. The area fraction of this row is row_sum / area_total
     * available; that fraction times the shorter side gives the
     * pixel thickness of the strip. */
    if (w <= h) {
        /* Shorter side is width — lay row vertically across height. */
        int strip_w = (int)((row_sum / total_area_remaining) * (double)w + 0.5);
        if (strip_w < 1) strip_w = 1;
        if (strip_w > w) strip_w = w;
        double inv_sum = row_sum > 0 ? 1.0 / row_sum : 0;
        int cur_y = rect->y0;
        for (int k = 0; k < n_row; k++) {
            int idx = idx_row[k];
            int cell_h = (k == n_row - 1)
                ? (rect->y1 - cur_y + 1)
                : (int)(areas[idx] * inv_sum * (double)h + 0.5);
            if (cell_h < 1) cell_h = 1;
            rects_out[idx].x0 = rect->x0;
            rects_out[idx].x1 = rect->x0 + strip_w - 1;
            rects_out[idx].y0 = cur_y;
            rects_out[idx].y1 = cur_y + cell_h - 1;
            if (rects_out[idx].y1 > rect->y1) rects_out[idx].y1 = rect->y1;
            cur_y = rects_out[idx].y1 + 1;
        }
        rect->x0 += strip_w;
    } else {
        /* Shorter side is height — lay row horizontally across width. */
        int strip_h = (int)((row_sum / total_area_remaining) * (double)h + 0.5);
        if (strip_h < 1) strip_h = 1;
        if (strip_h > h) strip_h = h;
        double inv_sum = row_sum > 0 ? 1.0 / row_sum : 0;
        int cur_x = rect->x0;
        for (int k = 0; k < n_row; k++) {
            int idx = idx_row[k];
            int cell_w = (k == n_row - 1)
                ? (rect->x1 - cur_x + 1)
                : (int)(areas[idx] * inv_sum * (double)w + 0.5);
            if (cell_w < 1) cell_w = 1;
            rects_out[idx].x0 = cur_x;
            rects_out[idx].x1 = cur_x + cell_w - 1;
            rects_out[idx].y0 = rect->y0;
            rects_out[idx].y1 = rect->y0 + strip_h - 1;
            if (rects_out[idx].x1 > rect->x1) rects_out[idx].x1 = rect->x1;
            cur_x = rects_out[idx].x1 + 1;
        }
        rect->y0 += strip_h;
    }
}

/* Squarify entry point. `areas` is value[i] already scaled into
 * pixel-area units (sum == initial rect area). `order` is the
 * descending-by-area index permutation. Writes pixel rects per
 * original index into `rects_out`. */
static void squarify(const double *areas, const int *order, int n,
                     treemap_rect rect, treemap_rect *rects_out)
{
    int row[FASTCHART_MAX_TREEMAP_ITEMS];
    int n_row = 0;
    double row_sum = 0, row_min = 0, row_max = 0;
    double total_remaining = 0;
    for (int i = 0; i < n; i++) total_remaining += areas[order[i]];

    int i = 0;
    while (i < n) {
        int idx = order[i];
        double v = areas[idx];

        int w = rect.x1 - rect.x0 + 1;
        int h = rect.y1 - rect.y0 + 1;
        double L = w <= h ? (double)h : (double)w;

        double cur_worst = n_row > 0
            ? worst_aspect(row_sum, row_min, row_max, L)
            : INFINITY;
        double new_min = (n_row == 0 || v < row_min) ? v : row_min;
        double new_max = (n_row == 0 || v > row_max) ? v : row_max;
        double new_worst = worst_aspect(row_sum + v, new_min, new_max, L);

        if (n_row == 0 || new_worst <= cur_worst) {
            row[n_row++] = idx;
            row_sum += v;
            row_min = new_min;
            row_max = new_max;
            i++;
        } else {
            place_row(areas, row, n_row, &rect, total_remaining, rects_out);
            total_remaining -= row_sum;
            n_row = 0;
            row_sum = row_min = row_max = 0;
            /* don't advance i — re-process the current item in a
             * fresh row in the new (smaller) rect. */
        }
    }
    if (n_row > 0) {
        place_row(areas, row, n_row, &rect, total_remaining, rects_out);
    }
}

int fastchart_treemap_render_to_target(fastchart_treemap_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    /* Reserve space for the optional title above the plot rect. */
    int top = 12;
    int title_h = 0;
    const char *title_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_TITLE);
    double base_size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double title_size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_TITLE, base_size * 1.4);
    if (self->title && ZSTR_LEN(self->title) > 0 && title_font) {
        if (fastchart_text_measure(t, title_font, title_size, ZSTR_VAL(self->title),
                                   NULL, &title_h, NULL, 0) == 0) {
            top += title_h + 10;
        }
    }

    /* Plot rect: full canvas minus title at top + 12px padding all
     * other sides. setPlotRect overrides per the usual base-class
     * contract (treemap layout is purely inside its rect, so we
     * honor the user's bounds verbatim). */
    int x0 = 12, y0 = top, x1 = W - 13, y1 = H - 13;
    if (self->has_plot_rect) {
        x0 = (int)self->plot_x0; y0 = (int)self->plot_y0;
        x1 = (int)self->plot_x1; y1 = (int)self->plot_y1;
    }
    if (x1 <= x0 || y1 <= y0) {
        zend_throw_error(NULL,
            "FastChart\\Treemap::draw() plot area is empty after layout reservation");
        return -1;
    }

    int n = self->item_count;
    if (n <= 0) {
        zend_throw_error(NULL,
            "FastChart\\Treemap::draw() requires setItems() with at least one item");
        return -1;
    }

    /* Sort indices by descending value. Insertion sort is fine for
     * the bounded n (<=256). Items with non-positive value are
     * dropped at setItems, so values here are guaranteed > 0. */
    int order[FASTCHART_MAX_TREEMAP_ITEMS];
    for (int i = 0; i < n; i++) order[i] = i;
    for (int i = 1; i < n; i++) {
        int key = order[i];
        double kv = self->items[key].value;
        int j = i - 1;
        while (j >= 0 && self->items[order[j]].value < kv) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }

    /* Convert values to pixel-area units so the squarify routine
     * works in pixel space directly. The total pixel area equals
     * the plot rect area; each cell gets value/total of it. */
    double total_value = 0;
    for (int i = 0; i < n; i++) total_value += self->items[i].value;
    if (total_value <= 0) {
        zend_throw_error(NULL,
            "FastChart\\Treemap::draw() requires at least one positive item value");
        return -1;
    }

    int W_px = x1 - x0 + 1;
    int H_px = y1 - y0 + 1;
    double area_total = (double)W_px * (double)H_px;
    double areas[FASTCHART_MAX_TREEMAP_ITEMS];
    for (int i = 0; i < n; i++) {
        areas[i] = self->items[i].value / total_value * area_total;
    }

    treemap_rect rect = { x0, y0, x1, y1 };
    treemap_rect rects[FASTCHART_MAX_TREEMAP_ITEMS];
    squarify(areas, order, n, rect, rects);

    /* Paint each cell + a 1px border. */
    int label_black = fastchart_target_color(t, 0, 0, 0, 0xFF);
    int label_white = fastchart_target_color(t, 255, 255, 255, 0xFF);

    const char *label_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double label_size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base_size);

    for (int i = 0; i < n; i++) {
        const fastchart_treemap_item *it = &self->items[i];
        int cell_color;
        int cell_rgb;
        if (it->color_rgb >= 0) {
            cell_color = fastchart_target_color_rgb(t, it->color_rgb);
            cell_rgb = it->color_rgb;
        } else {
            cell_color = pal.series[i % FASTCHART_PALETTE_SERIES_N];
            cell_rgb = (int)(fastchart_target_color_to_rgba(t, cell_color) & 0xFFFFFF);
        }
        int rx0 = rects[i].x0;
        int ry0 = rects[i].y0;
        int rw = rects[i].x1 - rects[i].x0 + 1;
        int rh = rects[i].y1 - rects[i].y0 + 1;
        fastchart_target_rect(t, rx0, ry0, rw, rh, cell_color, 1, 0);
        fastchart_target_rect(t, rx0, ry0, rw, rh, pal.border, 0, 1);

        /* Skip labels that won't fit. Cells smaller than ~3x the
         * font size in either dimension would have label glyphs
         * spilling across borders; we just leave those blank. */
        if (!self->show_labels || !label_font || !it->label) continue;
        int cell_w = rects[i].x1 - rects[i].x0 + 1;
        int cell_h = rects[i].y1 - rects[i].y0 + 1;
        int min_dim = (int)(label_size * 2.5);
        if (cell_w < min_dim || cell_h < min_dim) continue;

        int tw = 0, th = 0;
        if (fastchart_text_measure(t, label_font, label_size, it->label,
                                   &tw, &th, NULL, 0) != 0) continue;
        if (tw > cell_w - 6) continue;  /* would clip horizontally */

        /* Choose a foreground that contrasts with the cell color.
         * Luma threshold via the ITU-R BT.601 weights, computed off
         * the cell's source RGB. */
        int r = (cell_rgb >> 16) & 0xFF;
        int g = (cell_rgb >>  8) & 0xFF;
        int b =  cell_rgb        & 0xFF;
        int luma = (299 * r + 587 * g + 114 * b) / 1000;
        int label_color = luma > 145 ? label_black : label_white;

        int cx = (rects[i].x0 + rects[i].x1) / 2;
        int cy = (rects[i].y0 + rects[i].y1) / 2 + th / 2;
        fastchart_text_draw(t, label_font, label_size, label_color,
                            cx, cy, FASTCHART_ALIGN_CENTER,
                            it->label, NULL, 0);
    }

    /* Title last so it sits on top of any cell that might have
     * leaked into the top reservation under setPlotRect overrides. */
    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}

/* GD-only shim. */
int fastchart_treemap_render_to_image(fastchart_treemap_obj *self, gdImagePtr im)
{
    fastchart_target_t t;
    fastchart_target_from_gd(&t, im, self->dpi);
    return fastchart_treemap_render_to_target(self, &t);
}

ZEND_METHOD(FastChart_Treemap, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\Treemap::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_treemap_obj *self = Z_FASTCHART_TREEMAP_OBJ_P(ZEND_THIS);
    if (fastchart_treemap_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
