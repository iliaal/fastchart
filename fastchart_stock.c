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
    zend_long ts;
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

    zend_long ts;
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

int fastchart_stock_render_to_image(fastchart_obj *self, gdImagePtr im)
{
    if (Z_TYPE(self->data) != IS_ARRAY ||
        zend_hash_num_elements(Z_ARRVAL(self->data)) == 0) {
        zend_throw_error(NULL,
            "FastChart\\StockChart::draw() requires setOhlcv() with one or more rows");
        return -1;
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
        return -1;
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

    zend_long t_min = candles[0].ts;
    zend_long t_max = candles[n - 1].ts;
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
            zend_long pp;
            if (fastchart_zval_to_long(p, &pp) == 0 && pp >= 2 && pp <= n) {
                sma_periods[sma_count++] = (int)pp;
            }
        } ZEND_HASH_FOREACH_END();
    }

    /* Layout: full plot rect, then split into price + volume panes
     * if volume is enabled. Volume gets the bottom 22% of the plot. */
    fastchart_rect plot;
    fastchart_compute_layout(self, im, 1, 1, &plot);

    /* Pane stacking: optional volume + up to 3 indicator panes
     * stack below the price pane. Each sub-pane gets a fixed share
     * of the available height. */
    zval *cfg_panes = zend_hash_str_find(Z_ARRVAL(self->config),
                                         "indicator_panes",
                                         sizeof("indicator_panes") - 1);
    int n_indicator_panes = 0;
    if (cfg_panes && Z_TYPE_P(cfg_panes) == IS_ARRAY) {
        n_indicator_panes = (int)zend_hash_num_elements(Z_ARRVAL_P(cfg_panes));
        if (n_indicator_panes > 3) n_indicator_panes = 3;
    }

    fastchart_rect price_pane = plot;
    fastchart_rect volume_pane = { 0, 0, 0, 0 };
    fastchart_rect indicator_panes[3] = {{0,0,0,0},{0,0,0,0},{0,0,0,0}};

    {
        int total_h = plot.y1 - plot.y0;
        int gap = 6;
        int sub_count = (show_volume ? 1 : 0) + n_indicator_panes;
        if (sub_count > 0) {
            int sub_share = (int)(total_h * 0.20);
            if (sub_share < 30) sub_share = 30;
            int sub_h_each = (sub_share * sub_count > total_h * 60 / 100)
                ? (total_h * 60 / 100) / sub_count
                : sub_share;
            if (sub_h_each < 24) sub_h_each = 24;
            int total_sub_h = sub_h_each * sub_count + gap * sub_count;
            price_pane.y1 = plot.y1 - total_sub_h;

            int cur_y = price_pane.y1 + gap;
            int slot = 0;
            if (show_volume) {
                volume_pane.x0 = plot.x0;
                volume_pane.x1 = plot.x1;
                volume_pane.y0 = cur_y;
                volume_pane.y1 = cur_y + sub_h_each - 1;
                cur_y += sub_h_each + gap;
                slot++;
                (void)slot;
            }
            for (int p = 0; p < n_indicator_panes; p++) {
                indicator_panes[p].x0 = plot.x0;
                indicator_panes[p].x1 = plot.x1;
                indicator_panes[p].y0 = cur_y;
                indicator_panes[p].y1 = cur_y + sub_h_each - 1;
                cur_y += sub_h_each + gap;
            }
        }
    }

    fastchart_value_range yrange;
    if (self->y_axis_scale == FASTCHART_SCALE_LOG) {
        if (fastchart_value_range_compute_log(y_min, y_max, &yrange) != 0) {
            efree(candles);
            zend_value_error("FastChart\\StockChart::draw(): log Y-axis requires strictly-positive prices");
            return -1;
        }
    } else {
        fastchart_value_range_compute(y_min, y_max, 6, &yrange);
        fastchart_value_range_apply_override(self, &yrange);
    }

    fastchart_palette pal;
    fastchart_palette_init(im, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(im, self, &pal);

    fastchart_draw_frame(im, self, &plot, &pal);

    /* Sub-pane borders. Each pane gets its own rectangle so the
     * boundaries between price / volume / indicator are clear. */
    if (show_volume || n_indicator_panes > 0) {
        gdImageRectangle(im, price_pane.x0, price_pane.y0,
                         price_pane.x1, price_pane.y1, pal.border);
    }
    if (show_volume) {
        gdImageRectangle(im, volume_pane.x0, volume_pane.y0,
                         volume_pane.x1, volume_pane.y1, pal.border);
    }
    for (int p = 0; p < n_indicator_panes; p++) {
        gdImageRectangle(im,
            indicator_panes[p].x0, indicator_panes[p].y0,
            indicator_panes[p].x1, indicator_panes[p].y1,
            pal.border);
    }

    fastchart_draw_title(im, self, &plot, &pal);
    fastchart_draw_y_axis(im, self, &price_pane, &pal, &yrange);
    fastchart_draw_x_axis_time(im, self, &plot, &pal, t_min, t_max);
    fastchart_draw_axis_titles(im, self, &plot, &pal);

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

    int candle_style = (int)self->candle_style;

    /* Vector-candle precompute: 4-tuple per bar (volume strength
     * category) using the same algorithm as the pinescript "Vector
     * Candles" indicator. baseT is the rolling window for both the
     * volume average and the climax-volume max. va[i]: 0 = neutral,
     * 2 = rising (vol >= 1.5x avg), 1 = climax (vol >= 2x avg OR
     * volume*range >= max(volume*range over baseT)). For volume-
     * scaled bodies (STYLE_VOLUME) we just need the avg. */
    const int baseT = 10;
    int *va = NULL;
    double *vol_scale = NULL;

    /* Sliding-window state for rolling volume average over the previous
     * baseT bars. Both the vector and volume blocks consume the same
     * (sum, cnt) shape — keep one running window updated each step
     * instead of recomputing from scratch (was O(n*baseT)). */
    double win_sum = 0;
    int    win_cnt = 0;

    if (candle_style == FASTCHART_STYLE_VECTOR) {
        va = ecalloc(n, sizeof(int));
        for (int i = 0; i < n; i++) {
            /* Window for step i = [max(0,i-baseT) .. i-1]. */
            if (i >= 1 && candles[i - 1].has_volume) {
                win_sum += candles[i - 1].volume; win_cnt++;
            }
            if (i >= baseT + 1 && candles[i - 1 - baseT].has_volume) {
                win_sum -= candles[i - 1 - baseT].volume; win_cnt--;
            }
            if (!candles[i].has_volume) { va[i] = 0; continue; }
            double avg = win_cnt > 0 ? win_sum / win_cnt : candles[i].volume;
            /* Climax-max stays as a baseT-bounded inner pass; the window
             * shape is volume*range, not volume, and baseT=10 keeps the
             * constant factor negligible. */
            double climax = candles[i].volume * (candles[i].high - candles[i].low);
            double climax_max = 0;
            for (int k = 1; k <= baseT && i - k >= 0; k++) {
                if (!candles[i - k].has_volume) continue;
                double c = candles[i - k].volume *
                    (candles[i - k].high - candles[i - k].low);
                if (c > climax_max) climax_max = c;
            }
            if (avg > 0 && (candles[i].volume >= avg * 2 || climax >= climax_max)) {
                va[i] = 1;
            } else if (avg > 0 && candles[i].volume >= avg * 1.5) {
                va[i] = 2;
            } else {
                va[i] = 0;
            }
        }
    } else if (candle_style == FASTCHART_STYLE_VOLUME) {
        vol_scale = ecalloc(n, sizeof(double));
        for (int i = 0; i < n; i++) {
            if (i >= 1 && candles[i - 1].has_volume) {
                win_sum += candles[i - 1].volume; win_cnt++;
            }
            if (i >= baseT + 1 && candles[i - 1 - baseT].has_volume) {
                win_sum -= candles[i - 1 - baseT].volume; win_cnt--;
            }
            if (!candles[i].has_volume) { vol_scale[i] = 1.0; continue; }
            double avg = win_cnt > 0 ? win_sum / win_cnt : candles[i].volume;
            double r = avg > 0 ? candles[i].volume / avg : 1.0;
            /* Cap to [0.2x, 2.0x] of the base body width so a single
             * monster-volume bar doesn't blow the layout. */
            if (r < 0.2) r = 0.2;
            if (r > 2.0) r = 2.0;
            vol_scale[i] = r;
        }
    }

    /* Vector-candle palette: 6 colors for direction × strength. The
     * pinescript defaults are lime/cyan for bullish strong/rising and
     * red/fuchsia for bearish strong/rising. Neutral falls back to
     * the theme's up/down colors so a vector chart with no high-vol
     * bars looks like a normal candle chart. */
    int v_bull_strong  = 0, v_bull_rising = 0;
    int v_bear_strong  = 0, v_bear_rising = 0;
    if (candle_style == FASTCHART_STYLE_VECTOR) {
        v_bull_strong  = gdImageColorAllocate(im, 0x00, 0xE6, 0x40); /* lime */
        v_bull_rising  = gdImageColorAllocate(im, 0x05, 0xFF, 0xFB); /* cyan */
        v_bear_strong  = gdImageColorAllocate(im, 0xE3, 0x00, 0x00); /* red */
        v_bear_rising  = gdImageColorAllocate(im, 0xFF, 0x00, 0xFF); /* fuchsia */
    }

    for (int i = 0; i < n; i++) {
        int x = fastchart_x_time_to_pixel(&price_pane,
                                          candles[i].ts, t_min, t_max);
        int y_open  = fastchart_y_to_pixel(candles[i].open,  &yrange, &price_pane);
        int y_high  = fastchart_y_to_pixel(candles[i].high,  &yrange, &price_pane);
        int y_low   = fastchart_y_to_pixel(candles[i].low,   &yrange, &price_pane);
        int y_close = fastchart_y_to_pixel(candles[i].close, &yrange, &price_pane);

        int up = candles[i].close >= candles[i].open;
        int color = up ? pal.up : pal.down;
        if (candle_style == FASTCHART_STYLE_VECTOR) {
            if (up) {
                color = (va[i] == 1) ? v_bull_strong
                      : (va[i] == 2) ? v_bull_rising
                      : pal.up;
            } else {
                color = (va[i] == 1) ? v_bear_strong
                      : (va[i] == 2) ? v_bear_rising
                      : pal.down;
            }
        }

        /* Per-bar half-width: VOLUME mode scales by volume / avg. */
        int hw = half_w;
        if (candle_style == FASTCHART_STYLE_VOLUME && vol_scale) {
            hw = (int)((double)half_w * vol_scale[i] + 0.5);
            if (hw < 1) hw = 1;
        }

        switch (candle_style) {
            case FASTCHART_STYLE_BAR:
                /* Vertical line for high-low; left tick at open,
                 * right tick at close. Classic Western HLC bar. */
                gdImageLine(im, x, y_high, x, y_low, color);
                gdImageLine(im, x - half_w, y_open,  x, y_open,  color);
                gdImageLine(im, x, y_close, x + half_w, y_close, color);
                break;

            case FASTCHART_STYLE_DIAMOND: {
                /* High-low wick + small diamond at the close. */
                gdImageLine(im, x, y_high, x, y_low, color);
                int dh = half_w < 4 ? 4 : half_w;
                gdPoint pts[4] = {
                    { x,      y_close - dh },
                    { x + dh, y_close      },
                    { x,      y_close + dh },
                    { x - dh, y_close      },
                };
                gdImageFilledPolygon(im, pts, 4, color);
                break;
            }

            case FASTCHART_STYLE_I_CAP:
                /* High-low wick + horizontal cap at high and low. */
                gdImageLine(im, x, y_high, x, y_low, color);
                gdImageLine(im, x - half_w, y_high, x + half_w, y_high, color);
                gdImageLine(im, x - half_w, y_low,  x + half_w, y_low,  color);
                break;

            case FASTCHART_STYLE_HOLLOW: {
                /* Bullish: outline-only (hollow) body. Bearish: filled.
                 * Wick always drawn. TradingView-style hollow candles. */
                gdImageLine(im, x, y_high, x, y_low, color);
                int y_top = up ? y_close : y_open;
                int y_bot = up ? y_open  : y_close;
                if (y_top == y_bot) {
                    gdImageLine(im, x - half_w, y_top, x + half_w, y_top, color);
                } else if (up) {
                    gdImageRectangle(im, x - half_w, y_top,
                                     x + half_w, y_bot, color);
                } else {
                    gdImageFilledRectangle(im, x - half_w, y_top,
                                           x + half_w, y_bot, color);
                }
                break;
            }

            case FASTCHART_STYLE_VOLUME: {
                /* Standard candle, but the body width scales by
                 * volume / rolling-avg. Wick stays at the cell center. */
                gdImageLine(im, x, y_high, x, y_low, color);
                int y_top = up ? y_close : y_open;
                int y_bot = up ? y_open  : y_close;
                if (y_top == y_bot) {
                    gdImageLine(im, x - hw, y_top, x + hw, y_top, color);
                } else {
                    gdImageFilledRectangle(im, x - hw, y_top,
                                           x + hw, y_bot, color);
                }
                break;
            }

            case FASTCHART_STYLE_VECTOR: {
                /* Filled candle body with vector-derived color. The
                 * color was already selected above based on the
                 * volume-strength category. */
                gdImageLine(im, x, y_high, x, y_low, color);
                int y_top = up ? y_close : y_open;
                int y_bot = up ? y_open  : y_close;
                if (y_top == y_bot) {
                    gdImageLine(im, x - half_w, y_top, x + half_w, y_top, color);
                } else {
                    gdImageFilledRectangle(im, x - half_w, y_top,
                                           x + half_w, y_bot, color);
                }
                break;
            }

            case FASTCHART_STYLE_CANDLE:
            default: {
                /* High-low wick + filled body. Doji (open == close)
                 * collapses to a horizontal line at the open. */
                gdImageLine(im, x, y_high, x, y_low, color);
                int y_top = up ? y_close : y_open;
                int y_bot = up ? y_open  : y_close;
                if (y_top == y_bot) {
                    gdImageLine(im, x - half_w, y_top, x + half_w, y_top, color);
                } else {
                    gdImageFilledRectangle(im, x - half_w, y_top,
                                           x + half_w, y_bot, color);
                }
                break;
            }
        }
    }
    if (va) efree(va);
    if (vol_scale) efree(vol_scale);

    /* SMA overlays. Each period gets a series color and a 2px line.
     * gdAntiAliased smooths the diagonals; set the AA color per
     * overlay since gd carries one AA color across calls. */
    if (sma_count > 0) {
        gdImageSetThickness(im, 2);
        for (int s = 0; s < sma_count; s++) {
            int period = sma_periods[s];
            int color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            gdImageSetAntiAliased(im, color);
            int prev_x = 0, prev_y = 0;
            int has_prev = 0;

            /* Sliding-window sum: each step subtracts the value
             * leaving the window and adds the new one — O(n) over
             * the whole series, vs O(n*period) for the naive
             * recomputation. */
            double sum = 0;
            for (int k = 0; k < period && k < n; k++) sum += candles[k].close;
            double inv_p = 1.0 / (double)period;

            for (int i = period - 1; i < n; i++) {
                if (i >= period) sum += candles[i].close - candles[i - period].close;
                double avg = sum * inv_p;
                int x = fastchart_x_time_to_pixel(&price_pane,
                                                  candles[i].ts, t_min, t_max);
                int y = fastchart_y_to_pixel(avg, &yrange, &price_pane);
                if (has_prev) {
                    gdImageLine(im, prev_x, prev_y, x, y, gdAntiAliased);
                }
                prev_x = x;
                prev_y = y;
                has_prev = 1;
            }
        }
        gdImageSetThickness(im, 1);
    }

    /* Volume pane: filled bars from baseline up. Default color
     * tracks candle direction (up-day green, down-day red) but the
     * setVolumeColors() override (config[volume_colors], parallel
     * to candles) wins when present. */
    if (show_volume && v_max > 0) {
        zval *vol_colors_zv = zend_hash_str_find(Z_ARRVAL(self->config),
            "volume_colors", sizeof("volume_colors") - 1);
        HashTable *vol_colors_ht =
            (vol_colors_zv && Z_TYPE_P(vol_colors_zv) == IS_ARRAY)
                ? Z_ARRVAL_P(vol_colors_zv) : NULL;

        int baseline = volume_pane.y1;
        int vh = volume_pane.y1 - volume_pane.y0;
        int bw = candle_w;
        int half_bw = bw / 2;
        if (half_bw < 1) half_bw = 1;

        /* Pre-walk vol_colors_ht once into a flat int[] of resolved
         * gd color handles (or -1 for "use default up/down"). The
         * draw loop then reads from that array instead of calling
         * gdImageColorAllocate per candle. */
        int *vol_colors = NULL;
        if (vol_colors_ht) {
            vol_colors = ecalloc((size_t)n, sizeof(int));
            for (int i = 0; i < n; i++) vol_colors[i] = -1;
            for (int i = 0; i < n; i++) {
                zval *cv = zend_hash_index_find(vol_colors_ht, i);
                if (cv && Z_TYPE_P(cv) == IS_LONG) {
                    zend_long c = Z_LVAL_P(cv);
                    if (c >= 0 && c <= 0xFFFFFF) {
                        vol_colors[i] = gdImageColorAllocate(im,
                            (int)((c >> 16) & 0xFF),
                            (int)((c >>  8) & 0xFF),
                            (int)( c        & 0xFF));
                    }
                }
            }
        }

        for (int i = 0; i < n; i++) {
            if (!candles[i].has_volume) continue;
            int x = fastchart_x_time_to_pixel(&volume_pane,
                                              candles[i].ts, t_min, t_max);
            int top = baseline - (int)((double)vh * candles[i].volume / v_max);

            int color = candles[i].close >= candles[i].open ? pal.up : pal.down;
            if (vol_colors && vol_colors[i] >= 0) color = vol_colors[i];

            gdImageFilledRectangle(im,
                x - half_bw, top, x + half_bw, baseline - 1, color);
        }
        if (vol_colors) efree(vol_colors);
    }

    /* Indicator panes. Each pane has its own y-range computed from
     * the supplied values (bounded by optional 'min' and 'max' from
     * the opts dict). Drawn as a 2px line; reference line if
     * configured; pane name in the top-left corner. */
    if (n_indicator_panes > 0) {
        double size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
        const char *font = self->font_path ? ZSTR_VAL(self->font_path) : NULL;

        int slot = 0;
        zval *pane_zv;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(cfg_panes), pane_zv) {
            if (slot >= n_indicator_panes) break;
            if (Z_TYPE_P(pane_zv) != IS_ARRAY) { slot++; continue; }

            zval *values_zv = zend_hash_str_find(Z_ARRVAL_P(pane_zv),
                                                 "values", sizeof("values") - 1);
            zval *name_zv = zend_hash_str_find(Z_ARRVAL_P(pane_zv),
                                               "name", sizeof("name") - 1);
            if (!values_zv || Z_TYPE_P(values_zv) != IS_ARRAY) { slot++; continue; }

            /* Read the first n values out of the array (parallel to
             * candles). Missing or non-numeric entries become NaN
             * so the line skips that segment. */
            double *vals = ecalloc((size_t)n, sizeof(double));
            int valid = 0;
            double pmin = 0, pmax = 0;
            HashTable *vht = Z_ARRVAL_P(values_zv);
            for (int i = 0; i < n; i++) {
                zval *vv = zend_hash_index_find(vht, i);
                double d;
                if (vv && fastchart_zval_to_double(vv, &d) == 0 && isfinite(d)) {
                    vals[i] = d;
                    if (valid == 0) { pmin = pmax = d; valid = 1; }
                    else { if (d < pmin) pmin = d; if (d > pmax) pmax = d; }
                } else {
                    vals[i] = NAN;
                }
            }
            if (!valid) { efree(vals); slot++; continue; }

            /* Apply opts.min / opts.max clamps. */
            zval *opt;
            opt = zend_hash_str_find(Z_ARRVAL_P(pane_zv), "min", 3);
            if (opt && Z_TYPE_P(opt) == IS_DOUBLE) pmin = Z_DVAL_P(opt);
            opt = zend_hash_str_find(Z_ARRVAL_P(pane_zv), "max", 3);
            if (opt && Z_TYPE_P(opt) == IS_DOUBLE) pmax = Z_DVAL_P(opt);

            fastchart_value_range pr;
            fastchart_value_range_compute(pmin, pmax, 4, &pr);

            int color;
            opt = zend_hash_str_find(Z_ARRVAL_P(pane_zv), "color", 5);
            if (opt && Z_TYPE_P(opt) == IS_LONG) {
                long c = Z_LVAL_P(opt);
                color = gdImageColorAllocate(im,
                    (int)((c >> 16) & 0xFF),
                    (int)((c >>  8) & 0xFF),
                    (int)( c        & 0xFF));
            } else {
                color = pal.series[(slot + 4) % FASTCHART_PALETTE_SERIES_N];
            }

            /* Reference line, if configured. */
            opt = zend_hash_str_find(Z_ARRVAL_P(pane_zv), "reference", 9);
            if (opt && Z_TYPE_P(opt) == IS_DOUBLE) {
                double r = Z_DVAL_P(opt);
                int ry = fastchart_y_to_pixel(r, &pr, &indicator_panes[slot]);
                if (ry >= indicator_panes[slot].y0 && ry <= indicator_panes[slot].y1) {
                    static int ref_style[8];
                    for (int k = 0; k < 4; k++) ref_style[k] = pal.grid;
                    for (int k = 4; k < 8; k++) ref_style[k] = gdTransparent;
                    gdImageSetStyle(im, ref_style, 8);
                    gdImageLine(im, indicator_panes[slot].x0 + 1, ry,
                                    indicator_panes[slot].x1 - 1, ry, gdStyled);
                }
            }

            /* Polyline through the values that map to actual times. */
            gdImageSetThickness(im, 2);
            gdImageSetAntiAliased(im, color);
            int prev_x = 0, prev_y = 0;
            int has_prev = 0;
            for (int i = 0; i < n; i++) {
                if (isnan(vals[i])) { has_prev = 0; continue; }
                int x = fastchart_x_time_to_pixel(&indicator_panes[slot],
                                                  candles[i].ts, t_min, t_max);
                int y = fastchart_y_to_pixel(vals[i], &pr, &indicator_panes[slot]);
                if (has_prev) {
                    gdImageLine(im, prev_x, prev_y, x, y, gdAntiAliased);
                }
                prev_x = x;
                prev_y = y;
                has_prev = 1;
            }
            gdImageSetThickness(im, 1);

            /* Pane name top-left of the pane. */
            if (font && name_zv && Z_TYPE_P(name_zv) == IS_STRING) {
                fastchart_text_draw(im, font, size * 0.9, color,
                    indicator_panes[slot].x0 + 6,
                    indicator_panes[slot].y0 + (int)(size * 1.1),
                    FASTCHART_ALIGN_LEFT, Z_STRVAL_P(name_zv), NULL, 0);
            }

            efree(vals);
            slot++;
        } ZEND_HASH_FOREACH_END();
    }

    /* Combo overlays draw onto the price pane using the price y
     * range. Allocate a flat timestamps array since the helper
     * walks a contiguous long[] rather than the candle struct. */
    {
        zend_long *ts_arr = ecalloc((size_t)n, sizeof(zend_long));
        for (int i = 0; i < n; i++) ts_arr[i] = candles[i].ts;
        fastchart_draw_overlays_time(im, self, &price_pane, &pal, &yrange,
                                      t_min, t_max, ts_arr, n);
        efree(ts_arr);
    }

    /* Annotations: only Y annotations apply within the price pane;
     * X annotations apply across the whole chart. */
    fastchart_draw_h_annotations(im, self, &price_pane, &pal, &yrange);
    fastchart_draw_v_annotations_time(im, self, &plot, &pal, t_min, t_max);

    /* Legend for the SMA overlays. */
    if (sma_count > 0) {
        int legend_colors[8];
        const char *legend_labels[8];
        char *legend_label_storage = ecalloc((size_t)sma_count, 16);
        for (int s = 0; s < sma_count; s++) {
            legend_colors[s] = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            char *slot = legend_label_storage + s * 16;
            snprintf(slot, 16, "SMA(%d)", sma_periods[s]);
            legend_labels[s] = slot;
        }
        fastchart_draw_legend(im, self, &price_pane, &pal,
                              sma_count, legend_colors, legend_labels);
        efree(legend_label_storage);
    }

    fastchart_draw_text_annotations(im, self, &pal);
    efree(candles);
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
    if (fastchart_stock_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
