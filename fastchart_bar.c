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
#include "fastchart_effects.h"

/* Resolve a per-point color override into a target color HANDLE. The
 * fallback is also a target handle. Per-point RGB overrides flow
 * through fastchart_target_color_rgb so the handle table dedupes
 * repeats across all backends. */
static int bar_per_point_color(zend_long *point_colors, int idx, int fallback,
                               fastchart_target_t *t)
{
    if (!point_colors) return fallback;
    zend_long c = point_colors[idx];
    if (c < 0) return fallback;
    return fastchart_target_color_rgb(t, (int)c);
}

static int fastchart_bar_render_horizontal(fastchart_bar_obj *self,
                                           fastchart_target_t *t);

/* Data-range scan shared by the vertical and horizontal bar render
 * paths. Walks the series array three different ways depending on
 * stack/floating mode and returns the [dmin, dmax] envelope. Caller
 * supplies the already-resolved stacked/floating flags so the scan
 * doesn't re-apply them. Returns 0 on success, -1 if no numeric
 * values were found (caller throws). */
static int bar_compute_range(const fastchart_bar_obj *self,
                             bool stacked, bool floating,
                             double *out_dmin, double *out_dmax)
{
    const fastchart_series_t *series = self->series;
    int n_series = self->n_series;
    int n_categories = self->max_len;

    double dmin = 0, dmax = 0;
    int seen = 0;
    if (floating) {
        for (int s = 0; s < n_series; s++) {
            for (int i = 0; i < series[s].len; i++) {
                double lo = series[s].values[i];
                double hi = series[s].values_max ? series[s].values_max[i] : NAN;
                if (isnan(lo) || isnan(hi)) continue;
                if (!seen) { dmin = lo; dmax = hi; seen = 1; }
                else { if (lo < dmin) dmin = lo; if (hi > dmax) dmax = hi; }
            }
        }
    } else if (stacked && n_series > 1) {
        for (int i = 0; i < n_categories; i++) {
            double pos = 0, neg = 0;
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                if (v >= 0) pos += v; else neg += v;
            }
            if (!seen) { dmin = neg; dmax = pos; seen = 1; }
            else { if (pos > dmax) dmax = pos; if (neg < dmin) dmin = neg; }
        }
    } else {
        for (int s = 0; s < n_series; s++) {
            for (int i = 0; i < series[s].len; i++) {
                double d = series[s].values[i];
                if (isnan(d)) continue;
                if (!seen) { dmin = dmax = d; seen = 1; }
                else { if (d < dmin) dmin = d; if (d > dmax) dmax = d; }
            }
        }
    }
    if (!seen) return -1;
    /* Floating bars don't anchor at zero. Regular bars do. */
    if (!floating) {
        if (dmin > 0) dmin = 0;
        if (dmax < 0) dmax = 0;
    }
    *out_dmin = dmin;
    *out_dmax = dmax;
    return 0;
}

int fastchart_bar_render_to_target(fastchart_bar_obj *self, fastchart_target_t *t)
{
    if (self->n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\BarChart::draw() requires setSeries() to have been called with non-empty data");
        return -1;
    }
    if (self->bar_orientation == FASTCHART_BAR_HORIZONTAL) {
        return fastchart_bar_render_horizontal(self, t);
    }
    fastchart_series_t *series = self->series;
    int n_series = self->n_series;
    int n_categories = self->max_len;

    bool stacked = self->stacked;
    bool stack_layer = (self->stack_mode == FASTCHART_STACK_LAYER);
    if (self->stack_mode == FASTCHART_STACK_BESIDE) stacked = false;
    if (stack_layer && n_series > 1) stacked = true;
    bool floating = self->bar_floating;

    double dmin = 0, dmax = 0;
    if (bar_compute_range(self, stacked, floating, &dmin, &dmax) != 0) {
        zend_throw_error(NULL,
            "FastChart\\BarChart::draw() found no numeric values in the series");
        return -1;
    }

    fastchart_value_range range;
    if (self->y_axis_scale == FASTCHART_SCALE_LOG) {
        if (dmin <= 0) {
            zend_value_error("FastChart\\BarChart::draw(): log Y-axis requires strictly-positive data (bars anchor at 0)");
            return -1;
        }
        if (fastchart_value_range_compute_log(dmin, dmax, &range) != 0) {
            zend_value_error("FastChart\\BarChart::draw(): log Y-axis requires strictly-positive data");
            return -1;
        }
    } else {
        fastchart_value_range_compute(dmin, dmax, 6, &range);
        fastchart_value_range_apply_override((fastchart_obj *)self, &range);
    }

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, t, 1, 1, NULL, 0, &plot);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    fastchart_draw_frame(t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_y_axis(t, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_plot_bands(t, (fastchart_obj *)self, &plot, &range, &pal);
    fastchart_draw_v_plot_bands_categorical(t, (fastchart_obj *)self, &plot,
                                            n_categories, &pal);

    const char **label_ptrs = fastchart_borrow_category_labels((fastchart_obj *)self, n_categories);
    fastchart_draw_x_axis_categorical(t, (fastchart_obj *)self, &plot, &pal, n_categories, label_ptrs);
    if (label_ptrs) efree((void *)label_ptrs);

    fastchart_draw_axis_titles(t, (fastchart_obj *)self, &plot, &pal);

    int zero_y = fastchart_y_to_pixel(0.0, &range, &plot);

    int slot_w = (plot.x1 - plot.x0) / (n_categories > 0 ? n_categories : 1);
    int slot_pad = slot_w / 6;
    if (slot_pad < 1) slot_pad = 1;
    int slot_inner = slot_w - 2 * slot_pad;
    if (slot_inner < 1) slot_inner = 1;

    int sub_count = (stacked && n_series > 1) ? 1 : n_series;
    int sub_w = slot_inner / sub_count;
    if (sub_w < 1) sub_w = 1;

    /* setBarWidth(pct) shrinks the bar fill within its allocated
     * sub-slot, centered, at pct/100 of the slot width. 100 = touch
     * neighbors, 50 = half-width with breathing room. Applied to
     * sub_w so per-series side-by-side bars all narrow together. */
    int bar_pct = (int)self->bar_width_pct;
    if (bar_pct <= 0) bar_pct = 100;
    int draw_w = (sub_w * bar_pct + 50) / 100;
    if (draw_w < 1) draw_w = 1;
    int sub_inset = (sub_w - draw_w) / 2;

    int edge_rgb = (int)self->edge_color;
    int edge_handle = edge_rgb >= 0
        ? fastchart_target_color_rgb(t, edge_rgb) : -1;

    /* Gradient LUT cache: built lazily on the first per-bar gradient
     * fill, reused across the rest of the chart (~500x speedup on
     * 500-bar gradient charts). GD-only effect; SVG path skips it. */
    fastchart_gradient_cache grad_cache;
    fastchart_gradient_cache_reset(&grad_cache);

    /* Allocate translucent series colors once for STACK_LAYER mode.
     * Per-handle: unpack the palette rgba and reallocate with a
     * ~50% alpha so the layered overdraw is visible underneath.
     * Result handles flow through fastchart_target_rect on both
     * backends. */
    int layer_colors[FASTCHART_MAX_SERIES] = {0};
    if (stack_layer && n_series > 1) {
        for (int s = 0; s < n_series; s++) {
            uint32_t rgba = fastchart_target_color_to_rgba(t,
                pal.series[s % FASTCHART_PALETTE_SERIES_N]);
            int r = (rgba >> 16) & 0xFF;
            int g = (rgba >>  8) & 0xFF;
            int b =  rgba        & 0xFF;
            layer_colors[s] = fastchart_target_color(t, r, g, b, 127);
        }
        if (t->kind == FASTCHART_TARGET_GD) {
            gdImageAlphaBlending(t->u.gd.im, 1);
        }
    }

    /* Bool toggle for GD-only effects (gradient/shadow). The SVG
     * backend doesn't support these primitives in this round; bars
     * render as solid fills there. */
    bool gd = (t->kind == FASTCHART_TARGET_GD);
    gdImagePtr im = gd ? t->u.gd.im : NULL;

    for (int i = 0; i < n_categories; i++) {
        int slot_left = plot.x0 + i * slot_w + slot_pad;

        if (floating) {
            /* Floating bar: each series carries [min, max] per slot;
             * draw between min and max instead of from zero. */
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double lo = series[s].values[i];
                double hi = series[s].values_max ? series[s].values_max[i] : NAN;
                if (isnan(lo) || isnan(hi)) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = bar_per_point_color(series[s].point_colors, i, series_color, t);
                int y_lo = fastchart_y_to_pixel(lo, &range, &plot);
                int y_hi = fastchart_y_to_pixel(hi, &range, &plot);
                int y0 = y_hi < y_lo ? y_hi : y_lo;
                int y1 = y_hi < y_lo ? y_lo : y_hi;
                int x0 = slot_left + s * sub_w + sub_inset;
                int x1 = x0 + draw_w - 1;
                if (x1 > slot_left + slot_inner - 1) x1 = slot_left + slot_inner - 1;
                int painted = 0;
                if (gd) {
                    fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                    painted = fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, &grad_cache, x0, y0, x1, y1);
                }
                if (!painted) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, color, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                }
            }
        } else if (stack_layer && n_series > 1) {
            /* Layered: all series anchor at zero with translucent
             * fills, painter overlay rather than cumulative. */
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int color = layer_colors[s];
                int y_v = fastchart_y_to_pixel(v, &range, &plot);
                int y0 = y_v < zero_y ? y_v : zero_y;
                int y1 = y_v < zero_y ? zero_y : y_v;
                int x0 = slot_left + sub_inset;
                int x1 = x0 + draw_w - 1;
                if (x1 > slot_left + slot_inner - 1) x1 = slot_left + slot_inner - 1;
                int painted = 0;
                if (gd) {
                    fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                    painted = fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, &grad_cache, x0, y0, x1, y1);
                }
                if (!painted) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, color, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                }
            }
        } else if (stacked && n_series > 1) {
            double pos_acc = 0, neg_acc = 0;
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = bar_per_point_color(series[s].point_colors, i, series_color, t);

                double a, b;
                if (v >= 0) {
                    a = pos_acc; b = pos_acc + v; pos_acc = b;
                } else {
                    a = neg_acc + v; b = neg_acc; neg_acc = a;
                }
                int y_a = fastchart_y_to_pixel(a, &range, &plot);
                int y_b = fastchart_y_to_pixel(b, &range, &plot);
                int y0 = y_a < y_b ? y_a : y_b;
                int y1 = y_a < y_b ? y_b : y_a;
                int x0 = slot_left + sub_inset;
                int x1 = x0 + draw_w - 1;
                if (x1 > slot_left + slot_inner - 1) x1 = slot_left + slot_inner - 1;
                int painted = 0;
                if (gd) {
                    fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                    painted = fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, &grad_cache, x0, y0, x1, y1);
                }
                if (!painted) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, color, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                }
            }
        } else {
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = bar_per_point_color(series[s].point_colors, i, series_color, t);
                int y_v = fastchart_y_to_pixel(v, &range, &plot);

                int x0 = slot_left + s * sub_w + sub_inset;
                int x1 = x0 + draw_w - 1;
                if (x1 > slot_left + slot_inner - 1) x1 = slot_left + slot_inner - 1;

                int y0 = y_v < zero_y ? y_v : zero_y;
                int y1 = y_v < zero_y ? zero_y : y_v;
                int painted = 0;
                if (gd) {
                    fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                    painted = fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, &grad_cache, x0, y0, x1, y1);
                }
                if (!painted) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, color, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                }
            }
        }
    }

    if (range.min < 0 && range.max > 0) {
        fastchart_target_line(t, plot.x0, zero_y, plot.x1, zero_y,
                              pal.axis, 1, FASTCHART_DASH_SOLID);
    }

    /* Value labels above each bar (skipped when stacked since the
     * label would land mid-stack). */
    if (self->show_values && !(stacked && n_series > 1)) {
        for (int i = 0; i < n_categories; i++) {
            int slot_left = plot.x0 + i * slot_w + slot_pad;
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int y_v = fastchart_y_to_pixel(v, &range, &plot);
                int x0 = slot_left + s * sub_w;
                int x_center = x0 + sub_w / 2;
                /* Label sits just above the bar top (or below for
                 * negative bars). */
                int label_y = (v >= 0) ? y_v : y_v + (int)(self->font_size * 1.4);
                fastchart_draw_value_label(t, (fastchart_obj *)self, &pal, x_center, label_y, v);
            }
        }
    }

    fastchart_draw_overlays_categorical(t, (fastchart_obj *)self, &plot, &pal,
                                         &range, NULL, n_categories);

    fastchart_draw_h_annotations(t, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_v_annotations_categorical(t, (fastchart_obj *)self, &plot, &pal, n_categories);

    if (n_series >= 2) {
        int legend_colors[FASTCHART_MAX_SERIES];
        const char *legend_labels[FASTCHART_MAX_SERIES];
        int legend_count = 0;
        for (int s = 0; s < n_series; s++) {
            if (!series[s].label) continue;
            legend_colors[legend_count] = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
        if (legend_count > 0) {
            fastchart_draw_legend(t, (fastchart_obj *)self, &plot, &pal,
                                  legend_count, legend_colors, legend_labels);
        }
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);

    if (self->icons && self->n_icons > 0 && n_categories > 0) {
        for (int i = 0; i < self->n_icons; i++) {
            const fastchart_icon *ic = &self->icons[i];
            double frac_x = n_categories > 1
                ? (ic->x + 0.5) / (double)n_categories
                : 0.5;
            int px = plot.x0 + (int)(frac_x * (plot.x1 - plot.x0) + 0.5);
            int py = fastchart_y_to_pixel(ic->y, &range, &plot);
            fastchart_blit_icon(t, ic, px, py);
        }
    }
    return 0;
}

/* Horizontal-bar render path. Mirrors the vertical path with X/Y
 * swapped: categories run top-to-bottom along the Y axis, values run
 * left-to-right along the X axis, bars are horizontal rectangles
 * anchored at x=0. Stacking, floating, and per-point colors all carry
 * over with the obvious axis swap. Plot bands and value labels skip
 * the horizontal path for now (they assume a vertical chart). */
static int fastchart_bar_render_horizontal(fastchart_bar_obj *self,
                                           fastchart_target_t *t)
{
    fastchart_series_t *series = self->series;
    int n_series = self->n_series;
    int n_categories = self->max_len;

    bool stacked = self->stacked;
    bool stack_layer = (self->stack_mode == FASTCHART_STACK_LAYER);
    if (self->stack_mode == FASTCHART_STACK_BESIDE) stacked = false;
    if (stack_layer && n_series > 1) stacked = true;
    bool floating = self->bar_floating;

    double dmin = 0, dmax = 0;
    if (bar_compute_range(self, stacked, floating, &dmin, &dmax) != 0) {
        zend_throw_error(NULL,
            "FastChart\\BarChart::draw() found no numeric values in the series");
        return -1;
    }

    fastchart_value_range range;
    if (self->y_axis_scale == FASTCHART_SCALE_LOG) {
        if (dmin <= 0) {
            zend_value_error("FastChart\\BarChart::draw(): log axis requires strictly-positive data (bars anchor at 0)");
            return -1;
        }
        if (fastchart_value_range_compute_log(dmin, dmax, &range) != 0) {
            zend_value_error("FastChart\\BarChart::draw(): log axis requires strictly-positive data");
            return -1;
        }
    } else {
        fastchart_value_range_compute(dmin, dmax, 6, &range);
        fastchart_value_range_apply_override((fastchart_obj *)self, &range);
    }

    /* Borrow category labels up front so layout can size the left
     * margin to the widest one — categorical Y labels can be far
     * wider than the numeric "999999" probe (e.g. "/api/v2/exports").
     * Same buffer is then handed to the categorical Y-axis renderer. */
    const char **label_ptrs = fastchart_borrow_category_labels((fastchart_obj *)self, n_categories);

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, t, 1, 1,
                             label_ptrs, n_categories, &plot);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    fastchart_draw_frame(t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_title(t, (fastchart_obj *)self, &plot, &pal);
    fastchart_draw_x_axis_numeric(t, (fastchart_obj *)self, &plot, &pal, &range);

    fastchart_draw_y_axis_categorical(t, (fastchart_obj *)self, &plot, &pal, n_categories, label_ptrs);
    if (label_ptrs) efree((void *)label_ptrs);

    fastchart_draw_axis_titles(t, (fastchart_obj *)self, &plot, &pal);

    /* Plot bands: in the horizontal-bar layout the value axis is X
     * and the category axis is Y. The user-facing API names are
     * tied to the default vertical orientation, so the visual roles
     * swap here:
     *   - addVerticalBand (X-range entries) -> value-axis stripes
     *     via the xrange V-bands helper.
     *   - addHorizontalBand (Y-range entries on the default
     *     orientation) -> category-axis stripes via the new
     *     categorical H-bands helper, with low/high read as
     *     fractional category indices on the Y axis. */
    fastchart_draw_v_plot_bands_xrange(t, (fastchart_obj *)self, &plot,
                                       &range, &pal);
    fastchart_draw_h_plot_bands_categorical(t, (fastchart_obj *)self, &plot,
                                            n_categories, &pal);

    int zero_x = fastchart_x_to_pixel(0.0, &range, &plot);

    int slot_h = (plot.y1 - plot.y0) / (n_categories > 0 ? n_categories : 1);
    int slot_pad = slot_h / 6;
    if (slot_pad < 1) slot_pad = 1;
    int slot_inner = slot_h - 2 * slot_pad;
    if (slot_inner < 1) slot_inner = 1;

    int sub_count = (stacked && n_series > 1) ? 1 : n_series;
    int sub_h = slot_inner / sub_count;
    if (sub_h < 1) sub_h = 1;

    int bar_pct = (int)self->bar_width_pct;
    if (bar_pct <= 0) bar_pct = 100;
    int draw_h = (sub_h * bar_pct + 50) / 100;
    if (draw_h < 1) draw_h = 1;
    int sub_inset = (sub_h - draw_h) / 2;

    int edge_rgb = (int)self->edge_color;
    int edge_handle = edge_rgb >= 0
        ? fastchart_target_color_rgb(t, edge_rgb) : -1;

    fastchart_gradient_cache grad_cache;
    fastchart_gradient_cache_reset(&grad_cache);

    int layer_colors[FASTCHART_MAX_SERIES] = {0};
    if (stack_layer && n_series > 1) {
        for (int s = 0; s < n_series; s++) {
            uint32_t rgba = fastchart_target_color_to_rgba(t,
                pal.series[s % FASTCHART_PALETTE_SERIES_N]);
            int r = (rgba >> 16) & 0xFF;
            int g = (rgba >>  8) & 0xFF;
            int b =  rgba        & 0xFF;
            layer_colors[s] = fastchart_target_color(t, r, g, b, 127);
        }
        if (t->kind == FASTCHART_TARGET_GD) {
            gdImageAlphaBlending(t->u.gd.im, 1);
        }
    }

    bool gd = (t->kind == FASTCHART_TARGET_GD);
    gdImagePtr im = gd ? t->u.gd.im : NULL;

    for (int i = 0; i < n_categories; i++) {
        int slot_top = plot.y0 + i * slot_h + slot_pad;

        if (floating) {
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double lo = series[s].values[i];
                double hi = series[s].values_max ? series[s].values_max[i] : NAN;
                if (isnan(lo) || isnan(hi)) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = bar_per_point_color(series[s].point_colors, i, series_color, t);
                int x_lo = fastchart_x_to_pixel(lo, &range, &plot);
                int x_hi = fastchart_x_to_pixel(hi, &range, &plot);
                int x0 = x_lo < x_hi ? x_lo : x_hi;
                int x1 = x_lo < x_hi ? x_hi : x_lo;
                int y0 = slot_top + s * sub_h + sub_inset;
                int y1 = y0 + draw_h - 1;
                if (y1 > slot_top + slot_inner - 1) y1 = slot_top + slot_inner - 1;
                int painted = 0;
                if (gd) {
                    fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                    painted = fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, &grad_cache, x0, y0, x1, y1);
                }
                if (!painted) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, color, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                }
            }
        } else if (stack_layer && n_series > 1) {
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int color = layer_colors[s];
                int x_v = fastchart_x_to_pixel(v, &range, &plot);
                int x0 = x_v < zero_x ? x_v : zero_x;
                int x1 = x_v < zero_x ? zero_x : x_v;
                int y0 = slot_top + sub_inset;
                int y1 = y0 + draw_h - 1;
                if (y1 > slot_top + slot_inner - 1) y1 = slot_top + slot_inner - 1;
                int painted = 0;
                if (gd) {
                    fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                    painted = fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, &grad_cache, x0, y0, x1, y1);
                }
                if (!painted) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, color, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                }
            }
        } else if (stacked && n_series > 1) {
            double pos_acc = 0, neg_acc = 0;
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = bar_per_point_color(series[s].point_colors, i, series_color, t);
                double a, b;
                if (v >= 0) {
                    a = pos_acc; b = pos_acc + v; pos_acc = b;
                } else {
                    a = neg_acc + v; b = neg_acc; neg_acc = a;
                }
                int x_a = fastchart_x_to_pixel(a, &range, &plot);
                int x_b = fastchart_x_to_pixel(b, &range, &plot);
                int x0 = x_a < x_b ? x_a : x_b;
                int x1 = x_a < x_b ? x_b : x_a;
                int y0 = slot_top + sub_inset;
                int y1 = y0 + draw_h - 1;
                if (y1 > slot_top + slot_inner - 1) y1 = slot_top + slot_inner - 1;
                int painted = 0;
                if (gd) {
                    fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                    painted = fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, &grad_cache, x0, y0, x1, y1);
                }
                if (!painted) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, color, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                }
            }
        } else {
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
                int color = bar_per_point_color(series[s].point_colors, i, series_color, t);
                int x_v = fastchart_x_to_pixel(v, &range, &plot);

                int y0 = slot_top + s * sub_h + sub_inset;
                int y1 = y0 + draw_h - 1;
                if (y1 > slot_top + slot_inner - 1) y1 = slot_top + slot_inner - 1;

                int x0 = x_v < zero_x ? x_v : zero_x;
                int x1 = x_v < zero_x ? zero_x : x_v;
                int painted = 0;
                if (gd) {
                    fastchart_shadow_filled_rectangle(im, (fastchart_obj *)self, x0, y0, x1, y1);
                    painted = fastchart_gradient_filled_rectangle(im, (fastchart_obj *)self, &grad_cache, x0, y0, x1, y1);
                }
                if (!painted) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, color, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                }
            }
        }
    }

    if (range.min < 0 && range.max > 0) {
        fastchart_target_line(t, zero_x, plot.y0, zero_x, plot.y1,
                              pal.axis, 1, FASTCHART_DASH_SOLID);
    }

    /* Value labels next to each bar tip. Mirror of the vertical
     * path: skipped when stacked since the label would land
     * mid-stack. Positive bars get a label just past the bar's
     * right edge; negative bars to the left. */
    if (self->show_values && !(stacked && n_series > 1)) {
        for (int i = 0; i < n_categories; i++) {
            int slot_top = plot.y0 + i * slot_h + slot_pad;
            for (int s = 0; s < n_series; s++) {
                if (i >= series[s].len) continue;
                double v = series[s].values[i];
                if (isnan(v)) continue;
                int x_v = fastchart_x_to_pixel(v, &range, &plot);
                int y0 = slot_top + s * sub_h;
                int y_center = y0 + sub_h / 2;
                int label_x = (v >= 0) ? x_v + 4 : x_v - 4;
                fastchart_draw_value_label(t, (fastchart_obj *)self, &pal,
                                           label_x, y_center, v);
            }
        }
    }

    /* Combo overlays + annotations. The horizontal-bar helpers swap
     * X/Y from the vertical-bar pair: overlay polylines run with x =
     * value (xrange) and y = category center; "h" annotations
     * (addHorizontalLine, value-axis) become vertical screen lines;
     * "v" annotations (addVerticalLine, category-axis) become
     * horizontal screen lines. */
    fastchart_draw_overlays_horizontal_bar(t, (fastchart_obj *)self, &plot,
                                           &pal, &range, n_categories);
    fastchart_draw_horizontal_bar_annotations(t, (fastchart_obj *)self, &plot,
                                              &pal, &range, n_categories);

    if (n_series >= 2) {
        int legend_colors[FASTCHART_MAX_SERIES];
        const char *legend_labels[FASTCHART_MAX_SERIES];
        int legend_count = 0;
        for (int s = 0; s < n_series; s++) {
            if (!series[s].label) continue;
            legend_colors[legend_count] = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            legend_labels[legend_count] = series[s].label;
            legend_count++;
        }
        if (legend_count > 0) {
            fastchart_draw_legend(t, (fastchart_obj *)self, &plot, &pal,
                                  legend_count, legend_colors, legend_labels);
        }
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);

    /* Horizontal-bar IconPlot: x is the value (mapped via the X
     * value range), y is the fractional category index. Mirror of
     * the vertical-bar version. */
    if (self->icons && self->n_icons > 0 && n_categories > 0) {
        for (int i = 0; i < self->n_icons; i++) {
            const fastchart_icon *ic = &self->icons[i];
            double frac_y = n_categories > 1
                ? (ic->y + 0.5) / (double)n_categories
                : 0.5;
            int px = fastchart_x_to_pixel(ic->x, &range, &plot);
            int py = plot.y0 + (int)(frac_y * (plot.y1 - plot.y0) + 0.5);
            fastchart_blit_icon(t, ic, px, py);
        }
    }
    return 0;
}

/* GD-only shim for the legacy dispatch path (draw(\GdImage) and the
 * raster encoders that consume a gdImagePtr). Wrap the gd image in a
 * target and call the target-based renderer. */
int fastchart_bar_render_to_image(fastchart_bar_obj *self, gdImagePtr im)
{
    fastchart_target_t t;
    fastchart_target_from_gd(&t, im, self->dpi);
    return fastchart_bar_render_to_target(self, &t);
}

ZEND_METHOD(FastChart_BarChart, draw)
{
    zval *canvas_zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(canvas_zv, fastchart_gd_image_ce)
    ZEND_PARSE_PARAMETERS_END();

    gdImagePtr im = fastchart_gd_image_from_zval(canvas_zv);
    if (!im) {
        zend_throw_error(NULL, "FastChart\\BarChart::draw() received a closed or invalid GdImage");
        RETURN_THROWS();
    }
    if (!fastchart_require_truecolor(im)) RETURN_THROWS();

    fastchart_bar_obj *self = Z_FASTCHART_BAR_OBJ_P(ZEND_THIS);
    if (fastchart_bar_render_to_image(self, im) != 0) {
        RETURN_THROWS();
    }
    RETURN_ZVAL(canvas_zv, 1, 0);
}
