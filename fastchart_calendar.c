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

/* civil_from_days: inverse of the parser's days_from_civil.
 * Returns y/m/d through out-params. */
static void fastchart_civil_from_days(long z, int *y, int *m, int *d)
{
    z += 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    int yy = (int)yoe + (int)era * 400;
    unsigned doy = doe - (365 * yoe + yoe/4 - yoe/100);
    unsigned mp = (5 * doy + 2) / 153;
    unsigned dd = doy - (153 * mp + 2) / 5 + 1;
    unsigned mm = mp < 10 ? mp + 3 : mp - 9;
    if (mm <= 2) yy++;
    *y = yy;
    *m = (int)mm;
    *d = (int)dd;
}

/* day-of-week: 0=Sunday .. 6=Saturday. Compatible with the
 * GitHub-style calendar grid orientation (Sun on top). */
static int fastchart_dow_from_days(long z) {
    /* 1970-01-01 = Thursday = 4. */
    int dow = (int)((z + 4) % 7);
    if (dow < 0) dow += 7;
    return dow;
}

/* Calendar heatmap: 7 rows (Sun..Sat) × N week columns. Each cell
 * colored by value via low/high RGB ramp. Sparse data: missing days
 * render in the palette grid color. Year boundaries are shown via a
 * month-letter label row above the grid. */
int fastchart_calendar_render_to_target(fastchart_calendar_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (self->day_count <= 0) {
        zend_throw_error(NULL,
            "FastChart\\CalendarHeatmap::draw() requires setData() with at least one entry");
        return -1;
    }

    /* Range spans first → last (already sorted in setData). The grid
     * starts at the Sunday on/before the first day so the first
     * week column aligns with day-of-week rows. */
    long first = self->days[0].day;
    long last  = self->days[self->day_count - 1].day;
    int first_dow = fastchart_dow_from_days(first);
    long grid_start = first - first_dow;
    int total_days = (int)(last - grid_start + 1);
    int n_weeks = (total_days + 6) / 7;
    if (n_weeks < 1) n_weeks = 1;

    /* Value range for color ramp. */
    double vmin = self->days[0].value, vmax = self->days[0].value;
    for (int i = 1; i < self->day_count; i++) {
        double v = self->days[i].value;
        if (v < vmin) vmin = v;
        if (v > vmax) vmax = v;
    }
    if (vmax <= vmin) vmax = vmin + 1.0;

    int top_pad = 12;
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

    int left_pad = 36;     /* room for day-of-week letters */
    int right_pad = 12;
    int month_label_h = 16;
    int grid_x0 = left_pad;
    int grid_x1 = W - right_pad;
    int grid_y0 = top_pad + month_label_h + 4;
    int grid_y1 = H - 12;
    int avail_w = grid_x1 - grid_x0;
    int avail_h = grid_y1 - grid_y0;
    if (avail_w <= 14 || avail_h <= 7 * 6) {
        zend_throw_error(NULL,
            "FastChart\\CalendarHeatmap::draw() canvas is too small for the date range");
        return -1;
    }
    int cell_size = avail_w / n_weeks;
    int row_size  = avail_h / 7;
    if (row_size < cell_size) cell_size = row_size;
    if (cell_size > 18) cell_size = 18;
    if (cell_size < 4) cell_size = 4;
    int cell_pad = cell_size > 6 ? 1 : 0;

    /* Colors. */
    int lo_rgb = self->color_low_rgb  >= 0 ? self->color_low_rgb  : 0xDDEEFF;
    int hi_rgb = self->color_high_rgb >= 0 ? self->color_high_rgb : 0x1144AA;
    int lr = (lo_rgb >> 16) & 0xFF, lg = (lo_rgb >> 8) & 0xFF, lb = lo_rgb & 0xFF;
    int hr = (hi_rgb >> 16) & 0xFF, hg = (hi_rgb >> 8) & 0xFF, hb = hi_rgb & 0xFF;

    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size * 0.85);

    /* Day-of-week letters down the left edge. Mon/Wed/Fri only, to
     * match GitHub's compact convention. */
    if (font) {
        static const char *dow_labels[7] = { "S", "M", "T", "W", "T", "F", "S" };
        for (int r = 0; r < 7; r++) {
            if (r != 1 && r != 3 && r != 5) continue;
            int cy = grid_y0 + r * cell_size + cell_size / 2 + (int)(size * 0.4);
            fastchart_text_draw(t, font, size, pal.text,
                                grid_x0 - 6, cy, FASTCHART_ALIGN_RIGHT,
                                dow_labels[r], NULL, 0);
        }
    }

    /* Cells: walk every day from grid_start to last; index into
     * self->days via binary search (data is sorted). */
    int data_idx = 0;
    for (int w = 0; w < n_weeks; w++) {
        for (int r = 0; r < 7; r++) {
            long day = grid_start + (long)w * 7 + r;
            if (day < first || day > last) continue;
            /* Advance data_idx to first entry >= day. */
            while (data_idx < self->day_count && self->days[data_idx].day < day) {
                data_idx++;
            }
            int cx = grid_x0 + w * cell_size;
            int cy = grid_y0 + r * cell_size;
            int color;
            if (data_idx < self->day_count && self->days[data_idx].day == day) {
                double frac = (self->days[data_idx].value - vmin) / (vmax - vmin);
                if (frac < 0) frac = 0;
                if (frac > 1) frac = 1;
                int rr = lr + (int)((hr - lr) * frac);
                int gg = lg + (int)((hg - lg) * frac);
                int bb = lb + (int)((hb - lb) * frac);
                color = fastchart_target_color_rgb(
                    t, (rr << 16) | (gg << 8) | bb);
            } else {
                color = pal.grid;
            }
            fastchart_target_rect(t, cx + cell_pad, cy + cell_pad,
                                  cell_size - 2 * cell_pad,
                                  cell_size - 2 * cell_pad,
                                  color, 1, 0);
        }
        /* Month label only when day-1 of a month falls inside this
         * week (GitHub-style placement). Avoids stub labels on
         * partial leading columns — when grid_start lands mid-month,
         * week 0 spans two months but only the canonical "new month
         * starts here" label should appear, one cell to the right. */
        if (font) {
            int label_month = 0;
            for (int dr = 0; dr < 7; dr++) {
                long check_day = grid_start + (long)w * 7 + dr;
                if (check_day < first || check_day > last) continue;
                int yy, mm, dd;
                fastchart_civil_from_days(check_day, &yy, &mm, &dd);
                if (dd == 1) {
                    label_month = mm;
                    break;
                }
            }
            if (label_month) {
                static const char *month_labels[12] = {
                    "Jan","Feb","Mar","Apr","May","Jun",
                    "Jul","Aug","Sep","Oct","Nov","Dec"
                };
                int cx = grid_x0 + w * cell_size;
                fastchart_text_draw(t, font, size, pal.text,
                                    cx, grid_y0 - 4, FASTCHART_ALIGN_LEFT,
                                    month_labels[label_month - 1], NULL, 0);
            }
        }
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
