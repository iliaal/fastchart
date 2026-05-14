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

#include <time.h>
#include <math.h>

int fastchart_gantt_render_to_image(fastchart_gantt_obj *self, gdImagePtr im)
{
    fastchart_target_t t;
    fastchart_target_from_gd(&t, im, self->dpi);
    if (self->task_count == 0) {
        zend_throw_error(NULL,
            "FastChart\\GanttChart::draw() requires setTasks() with non-empty data");
        return -1;
    }
    fastchart_gantt_task *tasks = self->tasks;
    int n_tasks = self->task_count;
    zend_long t_min = tasks[0].start, t_max = tasks[0].end;
    for (int i = 1; i < n_tasks; i++) {
        if (tasks[i].start < t_min) t_min = tasks[i].start;
        if (tasks[i].end   > t_max) t_max = tasks[i].end;
    }

    if (self->gantt_has_range) {
        t_min = self->gantt_range_start;
        t_max = self->gantt_range_end;
    }
    if (t_max <= t_min) t_max = t_min + 86400;

    fastchart_palette pal;
    fastchart_palette_init(&t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(&t, (fastchart_obj *)self, &pal);

    fastchart_gradient_cache grad_cache;
    fastchart_gradient_cache_reset(&grad_cache);

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, &t, 1, 1, NULL, 0, &plot);

    /* Reserve a left margin for task name labels. */
    int label_pad = 0;
    if (self->gantt_show_labels) {
        const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
        if (font) {
            double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
            double size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base);
            int max_w = 0;
            for (int i = 0; i < n_tasks; i++) {
                if (!tasks[i].name) continue;
                int w = 0, h = 0;
                if (fastchart_text_measure(&t, font, size, tasks[i].name, &w, &h, NULL, 0) == 0) {
                    if (w > max_w) max_w = w;
                }
            }
            label_pad = max_w + 12;
        }
    }
    fastchart_rect bars = plot;
    bars.x0 += label_pad;

    fastchart_draw_frame(&t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(&t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_x_axis_time(&t, (fastchart_obj *)self, &bars, &pal, t_min, t_max);

    int rows = n_tasks;
    int row_h = (bars.y1 - bars.y0) / (rows > 0 ? rows : 1);
    if (row_h < 4) row_h = 4;
    int bar_h = row_h * 60 / 100;
    if (bar_h < 3) bar_h = 3;

    /* Per-row track separator + bars. */
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base);

    for (int i = 0; i < n_tasks; i++) {
        int row_y0 = bars.y0 + i * row_h;
        int row_yc = row_y0 + row_h / 2;
        gdImageLine(im, bars.x0, row_y0, bars.x1, row_y0, fastchart_target_color_to_gd(&t, pal.grid));

        int x_start = fastchart_x_time_to_pixel(&bars, tasks[i].start, t_min, t_max);
        int x_end   = fastchart_x_time_to_pixel(&bars, tasks[i].end,   t_min, t_max);
        if (x_end < x_start + 1) x_end = x_start + 1;

        int color = pal.series[i % FASTCHART_PALETTE_SERIES_N];
        if (tasks[i].color_rgb >= 0) {
            int rgb = (int)tasks[i].color_rgb;
            color = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        }

        if (tasks[i].is_milestone) {
            int s = bar_h;
            gdPoint diamond[4] = {
                { x_end,           row_yc - s/2 },
                { x_end + s/2,     row_yc       },
                { x_end,           row_yc + s/2 },
                { x_end - s/2,     row_yc       },
            };
            fastchart_shadow_filled_polygon(im, (fastchart_obj *)self, diamond, 4);
            gdImageFilledPolygon(im, diamond, 4, color);
            if (self->edge_color >= 0) gdImagePolygon(im, diamond, 4, (int)self->edge_color);
        } else {
            int y0 = row_yc - bar_h / 2;
            int y1 = row_yc + bar_h / 2;
            fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x_start, y0, x_end, y1);
            if (!fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, &grad_cache, x_start, y0, x_end, y1)) {
                gdImageFilledRectangle(im, x_start, y0, x_end, y1, color);
            }
            if (self->edge_color >= 0) gdImageRectangle(im, x_start, y0, x_end, y1, (int)self->edge_color);
        }

        if (self->gantt_show_labels && font && tasks[i].name) {
            int label_x = bars.x0 - 6;
            int label_y = row_yc + (int)(size * 0.35);
            fastchart_text_draw(&t, font, size, pal.text,
                                label_x, label_y, FASTCHART_ALIGN_RIGHT,
                                tasks[i].name, NULL, 0);
        }
    }

    /* Dependency arrows: line from each dep's end to this task's start. */
    for (int i = 0; i < n_tasks; i++) {
        for (int d = 0; d < tasks[i].n_deps; d++) {
            int di = tasks[i].deps[d];
            if (di < 0 || di >= n_tasks) continue;
            int ax = fastchart_x_time_to_pixel(&bars, tasks[di].end, t_min, t_max);
            int ay = bars.y0 + di * row_h + row_h / 2;
            int bx = fastchart_x_time_to_pixel(&bars, tasks[i].start, t_min, t_max);
            int by = bars.y0 + i  * row_h + row_h / 2;
            gdImageLine(im, ax, ay, ax + 6, ay, fastchart_target_color_to_gd(&t, pal.axis));
            gdImageLine(im, ax + 6, ay, ax + 6, by, fastchart_target_color_to_gd(&t, pal.axis));
            gdImageLine(im, ax + 6, by, bx, by, fastchart_target_color_to_gd(&t, pal.axis));
            /* Tiny arrowhead. */
            gdImageLine(im, bx, by, bx - 4, by - 3, fastchart_target_color_to_gd(&t, pal.axis));
            gdImageLine(im, bx, by, bx - 4, by + 3, fastchart_target_color_to_gd(&t, pal.axis));
        }
    }

    
    fastchart_draw_text_annotations(&t, (fastchart_obj *)self, &pal);
    return 0;
}

ZEND_METHOD(FastChart_GanttChart, draw)
{
    zval *canvas_zv;
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\GanttChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();
    fastchart_gantt_obj *self = Z_FASTCHART_GANTT_OBJ_P(ZEND_THIS);
    if (fastchart_gantt_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
