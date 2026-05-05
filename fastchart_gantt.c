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

#include <time.h>
#include <math.h>

#define MAX_TASKS 256

typedef struct {
    zend_long start;
    zend_long end;
    const char *name;
    zend_long color;   /* -1 = palette */
    bool milestone;
    int *deps;         /* indices into the array */
    int n_deps;
} fastchart_gantt_task;

static int collect_tasks(zval *data_zv, fastchart_gantt_task *out, int max_tasks,
                         int *out_count, zend_long *out_min, zend_long *out_max)
{
    *out_count = 0;
    *out_min = 0; *out_max = 0;
    int seen = 0;
    if (Z_TYPE_P(data_zv) != IS_ARRAY) return -1;

    zval *t;
    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(data_zv), t) {
        if (Z_TYPE_P(t) != IS_ARRAY) continue;
        if (*out_count >= max_tasks) break;
        HashTable *ht = Z_ARRVAL_P(t);
        zval *zs = zend_hash_str_find(ht, "start", sizeof("start") - 1);
        zval *ze = zend_hash_str_find(ht, "end",   sizeof("end") - 1);
        if (!zs || !ze) continue;
        zend_long s, e;
        if (fastchart_zval_to_long(zs, &s) != 0) continue;
        if (fastchart_zval_to_long(ze, &e) != 0) continue;
        if (e < s) { zend_long tmp = s; s = e; e = tmp; }

        out[*out_count].start = s;
        out[*out_count].end   = e;
        out[*out_count].deps  = NULL;
        out[*out_count].n_deps = 0;

        zval *zn = zend_hash_str_find(ht, "name", sizeof("name") - 1);
        out[*out_count].name = fastchart_label_or_null(zn);

        zval *zc = zend_hash_str_find(ht, "color", sizeof("color") - 1);
        out[*out_count].color = (zc && Z_TYPE_P(zc) == IS_LONG &&
            Z_LVAL_P(zc) >= 0 && Z_LVAL_P(zc) <= 0xFFFFFF) ? Z_LVAL_P(zc) : -1;

        zval *zm = zend_hash_str_find(ht, "milestone", sizeof("milestone") - 1);
        out[*out_count].milestone =
            (zm && (Z_TYPE_P(zm) == IS_TRUE ||
                    (Z_TYPE_P(zm) == IS_LONG && Z_LVAL_P(zm) != 0)));

        zval *zd = zend_hash_str_find(ht, "depends", sizeof("depends") - 1);
        if (zd && Z_TYPE_P(zd) == IS_ARRAY) {
            int n = (int)zend_hash_num_elements(Z_ARRVAL_P(zd));
            if (n > 0) {
                out[*out_count].deps = ecalloc(n, sizeof(int));
                int k = 0;
                zval *dv;
                ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zd), dv) {
                    if (Z_TYPE_P(dv) == IS_LONG) {
                        out[*out_count].deps[k++] = (int)Z_LVAL_P(dv);
                    }
                } ZEND_HASH_FOREACH_END();
                out[*out_count].n_deps = k;
            }
        }

        if (!seen) { *out_min = s; *out_max = e; seen = 1; }
        else { if (s < *out_min) *out_min = s; if (e > *out_max) *out_max = e; }
        (*out_count)++;
    } ZEND_HASH_FOREACH_END();
    return seen ? 0 : -1;
}

static void free_tasks(fastchart_gantt_task *tasks, int n)
{
    for (int i = 0; i < n; i++) {
        if (tasks[i].deps) efree(tasks[i].deps);
    }
}

int fastchart_gantt_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    fastchart_gantt_task tasks[MAX_TASKS];
    int n_tasks = 0;
    zend_long t_min = 0, t_max = 0;
    if (collect_tasks(&self->data, tasks, MAX_TASKS,
                      &n_tasks, &t_min, &t_max) != 0 || n_tasks == 0) {
        zend_throw_error(NULL,
            "FastChart\\GanttChart::draw() requires setTasks() with non-empty data");
        return -1;
    }

    if (self->gantt_has_range) {
        t_min = self->gantt_range_start;
        t_max = self->gantt_range_end;
    }
    if (t_max <= t_min) t_max = t_min + 86400;

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    fastchart_rect plot;
    fastchart_compute_layout(self, im, 1, 1, &plot);

    /* Reserve a left margin for task name labels. */
    int label_pad = 0;
    if (self->gantt_show_labels) {
        const char *font = fastchart_resolve_font(self, FC_FONT_LABEL);
        if (font) {
            double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
            double size = fastchart_resolve_font_size(self, FC_FONT_LABEL, base);
            int max_w = 0;
            for (int i = 0; i < n_tasks; i++) {
                if (!tasks[i].name) continue;
                int w = 0, h = 0;
                if (fastchart_text_measure(font, size, tasks[i].name, &w, &h, NULL, 0) == 0) {
                    if (w > max_w) max_w = w;
                }
            }
            label_pad = max_w + 12;
        }
    }
    fastchart_rect bars = plot;
    bars.x0 += label_pad;

    fastchart_draw_frame(im, self, &plot, &pal);
    fastchart_draw_title(im, self, &plot, &pal);
    fastchart_draw_x_axis_time(im, self, &bars, &pal, t_min, t_max);

    int rows = n_tasks;
    int row_h = (bars.y1 - bars.y0) / (rows > 0 ? rows : 1);
    if (row_h < 4) row_h = 4;
    int bar_h = row_h * 60 / 100;
    if (bar_h < 3) bar_h = 3;

    /* Per-row track separator + bars. */
    const char *font = fastchart_resolve_font(self, FC_FONT_LABEL);
    double base = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double size = fastchart_resolve_font_size(self, FC_FONT_LABEL, base);

    for (int i = 0; i < n_tasks; i++) {
        int row_y0 = bars.y0 + i * row_h;
        int row_yc = row_y0 + row_h / 2;
        gdImageLine(im, bars.x0, row_y0, bars.x1, row_y0, pal.grid);

        int x_start = fastchart_x_time_to_pixel(&bars, tasks[i].start, t_min, t_max);
        int x_end   = fastchart_x_time_to_pixel(&bars, tasks[i].end,   t_min, t_max);
        if (x_end < x_start + 1) x_end = x_start + 1;

        int color = pal.series[i % FASTCHART_PALETTE_SERIES_N];
        if (tasks[i].color >= 0) {
            int rgb = (int)tasks[i].color;
            color = gdImageColorAllocate(im,
                (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        }

        if (tasks[i].milestone) {
            int s = bar_h;
            gdPoint diamond[4] = {
                { x_end,           row_yc - s/2 },
                { x_end + s/2,     row_yc       },
                { x_end,           row_yc + s/2 },
                { x_end - s/2,     row_yc       },
            };
            fastchart_shadow_filled_polygon(im, self, diamond, 4);
            gdImageFilledPolygon(im, diamond, 4, color);
            if (self->edge_color >= 0) gdImagePolygon(im, diamond, 4, (int)self->edge_color);
        } else {
            int y0 = row_yc - bar_h / 2;
            int y1 = row_yc + bar_h / 2;
            fastchart_shadow_filled_rectangle(im, self, x_start, y0, x_end, y1);
            if (!fastchart_gradient_filled_rectangle(im, self, x_start, y0, x_end, y1)) {
                gdImageFilledRectangle(im, x_start, y0, x_end, y1, color);
            }
            if (self->edge_color >= 0) gdImageRectangle(im, x_start, y0, x_end, y1, (int)self->edge_color);
        }

        if (self->gantt_show_labels && font && tasks[i].name) {
            int label_x = bars.x0 - 6;
            int label_y = row_yc + (int)(size * 0.35);
            fastchart_text_draw(im, font, size, pal.text,
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
            gdImageLine(im, ax, ay, ax + 6, ay, pal.axis);
            gdImageLine(im, ax + 6, ay, ax + 6, by, pal.axis);
            gdImageLine(im, ax + 6, by, bx, by, pal.axis);
            /* Tiny arrowhead. */
            gdImageLine(im, bx, by, bx - 4, by - 3, pal.axis);
            gdImageLine(im, bx, by, bx - 4, by + 3, pal.axis);
        }
    }

    free_tasks(tasks, n_tasks);
    fastchart_draw_text_annotations(im, self, &pal);
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
    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);
    if (fastchart_gantt_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
