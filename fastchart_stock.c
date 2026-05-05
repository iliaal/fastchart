/*
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2026 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,     |
  | that is bundled with this package in the file LICENSE, and is       |
  | available through the world-wide-web at the following url:          |
  | http://www.php.net/license/3_01.txt                                 |
  +----------------------------------------------------------------------+
  | Author: Ilia Alshanetsky <ilia@ilia.ws>                              |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

/* Each candle has up to 6 components: timestamp, open, high, low,
 * close, volume. Volume is optional; missing or non-numeric drops
 * volume rendering for the bar but keeps the candle. */
typedef struct {
    long   ts;
    double open;
    double high;
    double low;
    double close;
    double volume;
    int    has_volume;
} fastchart_candle;

#define MAX_CANDLES 4096

/* Pull a candle out of one row. Required indices: 0=ts, 1=open,
 * 2=high, 3=low, 4=close. Optional 5=volume. Returns 0 on success. */
static int read_candle(zval *row, fastchart_candle *out)
{
    if (Z_TYPE_P(row) != IS_ARRAY) return -1;
    HashTable *r = Z_ARRVAL_P(row);
    if (zend_hash_num_elements(r) < 5) return -1;

    zval *zts = zend_hash_index_find(r, 0);
    zval *zo  = zend_hash_index_find(r, 1);
    zval *zh  = zend_hash_index_find(r, 2);
    zval *zl  = zend_hash_index_find(r, 3);
    zval *zc  = zend_hash_index_find(r, 4);
    if (!zts || !zo || !zh || !zl || !zc) return -1;

    long ts;
    double o, h, l, c;
    if (fastchart_zval_to_long(zts, &ts) != 0) return -1;
    if (fastchart_zval_to_double(zo, &o) != 0) return -1;
    if (fastchart_zval_to_double(zh, &h) != 0) return -1;
    if (fastchart_zval_to_double(zl, &l) != 0) return -1;
    if (fastchart_zval_to_double(zc, &c) != 0) return -1;
    if (!(isfinite(o) && isfinite(h) && isfinite(l) && isfinite(c))) return -1;

    out->ts = ts;
    out->open = o;
    out->high = h;
    out->low = l;
    out->close = c;
    out->volume = 0;
    out->has_volume = 0;

    zval *zv = zend_hash_index_find(r, 5);
    if (zv) {
        double v;
        if (fastchart_zval_to_double(zv, &v) == 0 && isfinite(v) && v >= 0) {
            out->volume = v;
            out->has_volume = 1;
        }
    }
    return 0;
}

ZEND_METHOD(FastChart_StockChart, draw)
{
    zval *canvas_zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\StockChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }

    fastchart_obj *self = Z_FASTCHART_OBJ_P(ZEND_THIS);

    if (Z_TYPE(self->data) != IS_ARRAY ||
        zend_hash_num_elements(Z_ARRVAL(self->data)) == 0) {
        zend_throw_error(NULL,
            "FastChart\\StockChart::draw() requires setOhlcv() with one or more rows");
        RETURN_THROWS();
    }

    fastchart_candle *candles = ecalloc(MAX_CANDLES, sizeof(fastchart_candle));
    int n = 0;
    int any_volume = 0;
    {
        zval *row;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL(self->data), row) {
            if (n >= MAX_CANDLES) break;
            if (read_candle(row, &candles[n]) != 0) continue;
            if (candles[n].has_volume) any_volume = 1;
            n++;
        } ZEND_HASH_FOREACH_END();
    }
    if (n == 0) {
        efree(candles);
        zend_throw_error(NULL,
            "FastChart\\StockChart::draw() found no valid OHLC rows; expected [ts, o, h, l, c] or [ts, o, h, l, c, v]");
        RETURN_THROWS();
    }

    /* Sort by timestamp. Insertion sort -- caller is expected to feed
     * already-sorted data so this is O(n) on the happy path; a worst-
     * case unsorted feed of 4096 candles is still fast (~16M compares
     * which run in single-digit ms on a modern CPU). */
    for (int i = 1; i < n; i++) {
        fastchart_candle k = candles[i];
        int j = i - 1;
        while (j >= 0 && candles[j].ts > k.ts) {
            candles[j + 1] = candles[j];
            j--;
        }
        candles[j + 1] = k;
    }

    long t_min = candles[0].ts;
    long t_max = candles[n - 1].ts;
    if (t_min == t_max) t_max = t_min + 1;  /* avoid div-by-zero */

    double y_min = candles[0].low, y_max = candles[0].high;
    double v_max = 0;
    for (int i = 0; i < n; i++) {
        if (candles[i].low  < y_min) y_min = candles[i].low;
        if (candles[i].high > y_max) y_max = candles[i].high;
        if (candles[i].has_volume && candles[i].volume > v_max) v_max = candles[i].volume;
    }

    /* Optional config: moving averages array, volume_pane bool. */
    bool show_volume = false;
    zval *cfg_vp = zend_hash_str_find(Z_ARRVAL(self->config),
                                      "volume_pane", sizeof("volume_pane") - 1);
    if (cfg_vp && Z_TYPE_P(cfg_vp) == IS_TRUE) show_volume = true;
    if (show_volume && !any_volume) show_volume = false;

    int sma_periods[8];
    int sma_count = 0;
    zval *cfg_ma = zend_hash_str_find(Z_ARRVAL(self->config),
                                      "moving_averages", sizeof("moving_averages") - 1);
    if (cfg_ma && Z_TYPE_P(cfg_ma) == IS_ARRAY) {
        zval *p;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(cfg_ma), p) {
            if (sma_count >= 8) break;
            long pp;
            if (fastchart_zval_to_long(p, &pp) == 0 && pp >= 2 && pp <= n) {
                sma_periods[sma_count++] = (int)pp;
            }
        } ZEND_HASH_FOREACH_END();
    }

    /* Layout: full plot rect, then split into price + volume panes
     * if volume is enabled. Volume gets the bottom 22% of the plot. */
    fastchart_rect plot;
    fastchart_compute_layout(self, im, 1, 1, &plot);

    fastchart_rect price_pane = plot;
    fastchart_rect volume_pane = { 0, 0, 0, 0 };
    if (show_volume) {
        int total_h = plot.y1 - plot.y0;
        int gap = 6;
        int volume_h = (int)(total_h * 0.22);
        if (volume_h < 30) volume_h = 30;
        price_pane.y1 = plot.y1 - volume_h - gap;
        volume_pane.x0 = plot.x0;
        volume_pane.x1 = plot.x1;
        volume_pane.y0 = plot.y1 - volume_h;
        volume_pane.y1 = plot.y1;
    }

    fastchart_value_range yrange;
    fastchart_value_range_compute(y_min, y_max, 6, &yrange);

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);

    fastchart_draw_frame(im, self, &plot, &pal);

    /* If volume sub-pane is on, draw a separator line and an inner
     * border. The price pane gets its own y-axis ticks; the volume
     * pane is unlabeled (for v0.1) -- just colored bars. */
    if (show_volume) {
        gdImageRectangle(im, price_pane.x0, price_pane.y0,
                         price_pane.x1, price_pane.y1, pal.border);
        gdImageRectangle(im, volume_pane.x0, volume_pane.y0,
                         volume_pane.x1, volume_pane.y1, pal.border);
    }

    fastchart_draw_title(im, self, &plot, &pal);
    fastchart_draw_y_axis(im, self, &price_pane, &pal, &yrange);
    fastchart_draw_x_axis_time(im, self, &plot, &pal, t_min, t_max);

    /* Candle width: divide the available x-span into (n + 1) cells
     * and use ~70% as the body width. Wicks always sit at the
     * center-x of the cell. */
    int candle_w;
    if (n > 1) {
        int span = price_pane.x1 - price_pane.x0;
        candle_w = (int)((double)span / (double)(n + 1) * 0.7);
        if (candle_w < 2) candle_w = 2;
        if (candle_w > 30) candle_w = 30;
    } else {
        candle_w = 12;
    }
    int half_w = candle_w / 2;
    if (half_w < 1) half_w = 1;

    for (int i = 0; i < n; i++) {
        int x = fastchart_x_time_to_pixel(&price_pane,
                                          candles[i].ts, t_min, t_max);
        int y_open  = fastchart_y_to_pixel(candles[i].open,  &yrange, &price_pane);
        int y_high  = fastchart_y_to_pixel(candles[i].high,  &yrange, &price_pane);
        int y_low   = fastchart_y_to_pixel(candles[i].low,   &yrange, &price_pane);
        int y_close = fastchart_y_to_pixel(candles[i].close, &yrange, &price_pane);

        int up = candles[i].close >= candles[i].open;
        int color = up ? pal.up : pal.down;

        /* Wick. */
        gdImageLine(im, x, y_high, x, y_low, color);

        /* Body. Hollow if up to mirror common candlestick conventions
         * (filled = down, hollow up = up); the clearer high-density
         * default is filled bodies for both with color discrimination,
         * so we use filled. Open and close are equal -> draw a single
         * horizontal line as a doji marker. */
        int y_top = up ? y_close : y_open;
        int y_bot = up ? y_open  : y_close;
        if (y_top == y_bot) {
            gdImageLine(im, x - half_w, y_top, x + half_w, y_top, color);
        } else {
            gdImageFilledRectangle(im, x - half_w, y_top,
                                   x + half_w, y_bot, color);
        }
    }

    /* SMA overlays. Each period gets a series color and a 2px line. */
    if (sma_count > 0) {
        gdImageSetThickness(im, 2);
        for (int s = 0; s < sma_count; s++) {
            int period = sma_periods[s];
            int color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            int prev_x = 0, prev_y = 0;
            int has_prev = 0;

            for (int i = period - 1; i < n; i++) {
                double sum = 0;
                for (int k = 0; k < period; k++) {
                    sum += candles[i - k].close;
                }
                double avg = sum / period;
                int x = fastchart_x_time_to_pixel(&price_pane,
                                                  candles[i].ts, t_min, t_max);
                int y = fastchart_y_to_pixel(avg, &yrange, &price_pane);
                if (has_prev) {
                    gdImageLine(im, prev_x, prev_y, x, y, color);
                }
                prev_x = x;
                prev_y = y;
                has_prev = 1;
            }
        }
        gdImageSetThickness(im, 1);
    }

    /* Volume pane: filled bars from baseline up. Bar color tracks
     * candle direction so up-day volume reads green, down-day red. */
    if (show_volume && v_max > 0) {
        int baseline = volume_pane.y1;
        int vh = volume_pane.y1 - volume_pane.y0;
        int bw = candle_w;
        int half_bw = bw / 2;
        if (half_bw < 1) half_bw = 1;
        for (int i = 0; i < n; i++) {
            if (!candles[i].has_volume) continue;
            int x = fastchart_x_time_to_pixel(&volume_pane,
                                              candles[i].ts, t_min, t_max);
            int top = baseline - (int)((double)vh * candles[i].volume / v_max);
            int color = candles[i].close >= candles[i].open ? pal.up : pal.down;
            gdImageFilledRectangle(im,
                x - half_bw, top, x + half_bw, baseline - 1, color);
        }
    }

    efree(candles);
    RETURN_ZVAL(canvas_zv, 1, 0);
}
