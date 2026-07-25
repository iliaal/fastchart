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
#include "fastchart_text.h"

/* Emit one filled bar rect, honoring chart-wide drop shadow and
 * gradient settings. Shadow paints first (so the main shape overlays
 * it); gradient replaces the solid color when active. */
static inline void bar_emit_filled_rect(fastchart_target_t *t,
                                         fastchart_obj *chart,
                                         int x0, int y0, int x1, int y1,
                                         int color)
{
    fastchart_shadow_filled_rectangle(t, chart, x0, y0, x1, y1);
    if (chart->gradient_from >= 0 && chart->gradient_to >= 0) {
        fastchart_target_gradient_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1,
                                        (uint32_t)chart->gradient_from,
                                        (uint32_t)chart->gradient_to,
                                        (int)chart->gradient_dir);
    } else {
        fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1,
                              color, 1, 0);
    }
}

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

static bool bar_category_has_data(const fastchart_series_t *series,
                                  int n_series, int category, bool floating)
{
    for (int s = 0; s < n_series; s++) {
        if (category >= series[s].len || isnan(series[s].values[category])) {
            continue;
        }
        if (!floating || (series[s].values_max &&
                          !isnan(series[s].values_max[category]))) {
            return true;
        }
    }
    return false;
}

typedef struct {
    int left;   /* left edge of the paddable inner region */
    int inner;  /* inner width available for bars */
    int sub;    /* per-series sub-slot size */
    int draw;   /* drawn bar thickness within a sub-slot */
    int inset;  /* centering inset within a sub-slot */
} fastchart_bar_slot;

/* Per-category slot geometry. Boundaries are computed in double so bar
 * centers track fastchart_x_categorical_center() (which places labels
 * and gridlines) instead of drifting: a single truncated integer slot
 * width accumulated as i*slot_w pushes the last bar tens of pixels off
 * its label at high category counts. Works for either axis by passing
 * the axis span endpoints. */
static fastchart_bar_slot fastchart_bar_slot_geom(int axis_lo, int axis_hi,
                                                  int i, int n,
                                                  int sub_count, int bar_pct)
{
    fastchart_bar_slot g;
    int span = axis_hi - axis_lo;
    int b0 = axis_lo + (int)((double)span * i / n);
    int b1 = axis_lo + (int)((double)span * (i + 1) / n);
    int slot = b1 - b0;
    int pad = slot / 6;
    if (pad < 1) pad = 1;
    int inner = slot - 2 * pad;
    if (inner < 1) inner = 1;
    int sub = inner / (sub_count > 0 ? sub_count : 1);
    if (sub < 1) sub = 1;
    int draw = (sub * bar_pct + 50) / 100;
    if (draw < 1) draw = 1;
    g.left = b0 + pad;
    g.inner = inner;
    g.sub = sub;
    g.draw = draw;
    g.inset = (sub - draw) / 2;
    return g;
}

static int fastchart_bar_render_horizontal(fastchart_bar_obj *self,
                                           fastchart_target_t *t);
static int fastchart_bar_render_radial(fastchart_bar_obj *self,
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
                fastchart_range_update(d, &dmin, &dmax, &seen);
            }
        }
    }
    fastchart_obj *base = (fastchart_obj *)self;
    for (int o = 0; o < base->n_combo_overlays; o++) {
        const fastchart_combo_overlay *ov = &base->combo_overlays[o];
        int lim = ov->n < n_categories ? ov->n : n_categories;
        for (int i = 0; i < lim; i++) {
            double d = ov->values[i];
            if (!isfinite(d)) continue;
            fastchart_range_update(d, &dmin, &dmax, &seen);
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
    /* Clear hot-spots from a prior successful render up front, so a render
     * that aborts on a validation error (e.g. log Y-axis with non-positive
     * data) does not leave stale areas behind for getImageMap(). The
     * success path resets and repopulates them again below. */
    fastchart_reset_image_map_areas((fastchart_obj *)self);
    if (self->n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\BarChart::draw() requires setSeries() to have been called with non-empty data");
        return -1;
    }
    if (self->bar_orientation == FASTCHART_BAR_HORIZONTAL) {
        return fastchart_bar_render_horizontal(self, t);
    }
    if (self->bar_orientation == FASTCHART_BAR_RADIAL) {
        return fastchart_bar_render_radial(self, t);
    }
    fastchart_series_t *series = self->series;
    int n_series = self->n_series;
    int n_categories = self->max_len;

    bool stacked = self->stacked;
    bool stack_layer = (self->stack_mode == FASTCHART_STACK_LAYER);
    if (self->stack_mode == FASTCHART_STACK_BESIDE) stacked = false;
    if (stack_layer && n_series > 1) stacked = true;
    bool floating = self->bar_floating;
    /* Floating bars always render side-by-side — the [lo,hi] pair already
     * encodes each bar's extent, so stacking has no meaning. Left on, the
     * collapsed sub-slot count pushes every series past the slot edge and
     * emits negative-width rects (invalid SVG, series invisible). */
    if (floating) { stacked = false; stack_layer = false; }

    /* STACK_LAYER draws each series independently from the baseline, so
     * the Y range must be the per-series extent, not the stacked sum.
     * Keep `stacked` for the render/sub-count logic but compute the
     * range with stacked semantics off in layered mode. */
    double dmin = 0, dmax = 0;
    if (bar_compute_range(self, stacked && !stack_layer, floating, &dmin, &dmax) != 0) {
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
		if (fastchart_value_range_apply_override((fastchart_obj *)self,
				&range) != 0) {
			return -1;
		}
    }

    fastchart_rect plot;
    fastchart_palette pal;
    fastchart_render_cartesian_setup((fastchart_obj *)self, t,
                                     1, 1, NULL, 0, &plot, &pal);

    fastchart_draw_y_axis(t, (fastchart_obj *)self, &plot, &pal, &range);
    fastchart_draw_plot_bands(t, (fastchart_obj *)self, &plot, &range, &pal);
    fastchart_draw_v_plot_bands_categorical(t, (fastchart_obj *)self, &plot,
                                            n_categories, &pal);

    const char **label_ptrs = fastchart_borrow_category_labels((fastchart_obj *)self, n_categories);
    fastchart_draw_x_axis_categorical(t, (fastchart_obj *)self, &plot, &pal, n_categories, label_ptrs);
    if (label_ptrs) efree((void *)label_ptrs);

	fastchart_draw_axis_titles(t, (fastchart_obj *)self, &plot, &pal);

	fastchart_reset_image_map_areas((fastchart_obj *)self);
	if (self->n_image_map_entries > 0) {
		fastchart_reserve_image_map_areas((fastchart_obj *)self, n_categories);
	}

	int zero_y = fastchart_y_to_pixel(0.0, &range, &plot);

    int sub_count = (stacked && n_series > 1) ? 1 : n_series;

    /* setBarWidth(pct) shrinks the bar fill within its allocated
     * sub-slot, centered, at pct/100 of the slot width. 100 = touch
     * neighbors, 50 = half-width with breathing room. Applied to
     * sub_w so per-series side-by-side bars all narrow together. */
    int bar_pct = (int)self->bar_width_pct;
    if (bar_pct <= 0) bar_pct = 100;

    int edge_rgb = (int)self->edge_color;
    int edge_handle = edge_rgb >= 0
        ? fastchart_target_color_rgb(t, edge_rgb) : -1;

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
    }

    for (int i = 0; i < n_categories; i++) {
        fastchart_bar_slot g = fastchart_bar_slot_geom(plot.x0, plot.x1,
            i, n_categories, sub_count, bar_pct);
        int slot_left = g.left;
        int slot_inner = g.inner;
        int sub_w = g.sub;
        int draw_w = g.draw;
        int sub_inset = g.inset;

        /* One hot-spot per category column. Covers the full plot
         * height so any click in the column registers on the bar's
         * data point — more usable than a tight bar-bounding-box,
         * especially for very short bars. */
        if (self->n_image_map_entries > i &&
            bar_category_has_data(series, n_series, i, floating)) {
            fastchart_push_image_map_rect((fastchart_obj *)self, i,
                slot_left, plot.y0, slot_inner, plot.y1 - plot.y0);
        }

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
                if (x1 < x0) continue;
                if (self->bar_style == FASTCHART_BAR_STYLE_DUMBBELL) {
                    /* Connector between the [min,max] pair with a filled
                     * circle at each end. */
                    int x_center = (x0 + x1) / 2;
                    int bullet_r = draw_w / 2;
                    if (bullet_r < 3) bullet_r = 3;
                    if (bullet_r > 12) bullet_r = 12;
                    fastchart_target_line(t, x_center, y0, x_center, y1,
                                          color, 2, FASTCHART_DASH_SOLID);
                    fastchart_target_ellipse(t, x_center, y0, bullet_r, bullet_r, color, 1, 0);
                    fastchart_target_ellipse(t, x_center, y1, bullet_r, bullet_r, color, 1, 0);
                    if (edge_handle >= 0) {
                        fastchart_target_ellipse(t, x_center, y0, bullet_r, bullet_r, edge_handle, 0, 1);
                        fastchart_target_ellipse(t, x_center, y1, bullet_r, bullet_r, edge_handle, 0, 1);
                    }
                } else {
                    bar_emit_filled_rect(t, (fastchart_obj *)self, x0, y0, x1, y1, color);
                    if (edge_handle >= 0) {
                        fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                    }
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
                bar_emit_filled_rect(t, (fastchart_obj *)self, x0, y0, x1, y1, color);
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
                bar_emit_filled_rect(t, (fastchart_obj *)self, x0, y0, x1, y1, color);
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
                if (x1 < x0) continue;

                if (self->bar_style == FASTCHART_BAR_STYLE_LOLLIPOP) {
                    /* Thin stem from the zero baseline to the value with
                     * a filled circle bullet at the value. */
                    int x_center = (x0 + x1) / 2;
                    int bullet_r = draw_w / 2;
                    if (bullet_r < 3) bullet_r = 3;
                    if (bullet_r > 12) bullet_r = 12;
                    fastchart_target_line(t, x_center, zero_y, x_center, y_v,
                                          color, 2, FASTCHART_DASH_SOLID);
                    fastchart_target_ellipse(t, x_center, y_v, bullet_r, bullet_r, color, 1, 0);
                    if (edge_handle >= 0) {
                        fastchart_target_ellipse(t, x_center, y_v, bullet_r, bullet_r, edge_handle, 0, 1);
                    }
                } else {
                    int y0 = y_v < zero_y ? y_v : zero_y;
                    int y1 = y_v < zero_y ? zero_y : y_v;
                    bar_emit_filled_rect(t, (fastchart_obj *)self, x0, y0, x1, y1, color);
                    if (edge_handle >= 0) {
                        fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                    }
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
            fastchart_bar_slot g = fastchart_bar_slot_geom(plot.x0, plot.x1,
                i, n_categories, sub_count, bar_pct);
            int slot_left = g.left;
            int sub_w = g.sub;
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

    const char *legend_labels[FASTCHART_MAX_SERIES];
    for (int s = 0; s < n_series; s++) legend_labels[s] = series[s].label;
    fastchart_draw_series_legend(t, (fastchart_obj *)self, &plot, &pal,
                                 n_series, legend_labels);

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);

    if (self->icons && self->n_icons > 0 && n_categories > 0) {
        for (int i = 0; i < self->n_icons; i++) {
            const fastchart_icon *ic = &self->icons[i];
            double frac_x = n_categories > 1
                ? (ic->x + 0.5) / (double)n_categories
                : 0.5;
            int px = fastchart_frac_to_px(frac_x, plot.x0, plot.x1);
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
    /* Floating bars always render side-by-side — see the vertical
     * renderer for the negative-width failure this prevents. */
    if (floating) { stacked = false; stack_layer = false; }

    /* STACK_LAYER draws each series independently from the baseline, so
     * the Y range must be the per-series extent, not the stacked sum.
     * Keep `stacked` for the render/sub-count logic but compute the
     * range with stacked semantics off in layered mode. */
    double dmin = 0, dmax = 0;
    if (bar_compute_range(self, stacked && !stack_layer, floating, &dmin, &dmax) != 0) {
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
		if (fastchart_value_range_apply_override((fastchart_obj *)self,
				&range) != 0) {
			return -1;
		}
    }

    /* Borrow category labels up front so layout can size the left
     * margin to the widest one — categorical Y labels can be far
     * wider than the numeric "999999" probe (e.g. "/api/v2/exports").
     * Same buffer is then handed to the categorical Y-axis renderer. */
    const char **label_ptrs = fastchart_borrow_category_labels((fastchart_obj *)self, n_categories);

    fastchart_rect plot;
    fastchart_palette pal;
    fastchart_render_cartesian_setup((fastchart_obj *)self, t, 1, 1,
                                     label_ptrs, n_categories, &plot, &pal);
    fastchart_draw_x_axis_numeric(t, (fastchart_obj *)self, &plot, &pal, &range);

    fastchart_draw_y_axis_categorical(t, (fastchart_obj *)self, &plot, &pal, n_categories, label_ptrs);
    if (label_ptrs) efree((void *)label_ptrs);

    fastchart_draw_axis_titles(t, (fastchart_obj *)self, &plot, &pal);

    fastchart_reset_image_map_areas((fastchart_obj *)self);
    if (self->n_image_map_entries > 0) {
        fastchart_reserve_image_map_areas((fastchart_obj *)self, n_categories);
    }

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

    int sub_count = (stacked && n_series > 1) ? 1 : n_series;

    int bar_pct = (int)self->bar_width_pct;
    if (bar_pct <= 0) bar_pct = 100;

    int edge_rgb = (int)self->edge_color;
    int edge_handle = edge_rgb >= 0
        ? fastchart_target_color_rgb(t, edge_rgb) : -1;

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
    }

    for (int i = 0; i < n_categories; i++) {
        fastchart_bar_slot g = fastchart_bar_slot_geom(plot.y0, plot.y1,
            i, n_categories, sub_count, bar_pct);
        int slot_top = g.left;
        int slot_inner = g.inner;
        int sub_h = g.sub;
        int draw_h = g.draw;
        int sub_inset = g.inset;

        /* One hot-spot per category row — full plot width, mirroring
         * the vertical path's full-height column rects. */
        if (self->n_image_map_entries > i &&
            bar_category_has_data(series, n_series, i, floating)) {
            fastchart_push_image_map_rect((fastchart_obj *)self, i,
                plot.x0, slot_top, plot.x1 - plot.x0, slot_inner);
        }

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
                if (y1 < y0) continue;
                if (self->bar_style == FASTCHART_BAR_STYLE_DUMBBELL) {
                    /* Horizontal connector across the [min,max] pair with a
                     * filled circle at each end (vertical dumbbell, X/Y
                     * swapped). */
                    int y_center = (y0 + y1) / 2;
                    int bullet_r = draw_h / 2;
                    if (bullet_r < 3) bullet_r = 3;
                    if (bullet_r > 12) bullet_r = 12;
                    fastchart_target_line(t, x0, y_center, x1, y_center,
                                          color, 2, FASTCHART_DASH_SOLID);
                    fastchart_target_ellipse(t, x0, y_center, bullet_r, bullet_r, color, 1, 0);
                    fastchart_target_ellipse(t, x1, y_center, bullet_r, bullet_r, color, 1, 0);
                    if (edge_handle >= 0) {
                        fastchart_target_ellipse(t, x0, y_center, bullet_r, bullet_r, edge_handle, 0, 1);
                        fastchart_target_ellipse(t, x1, y_center, bullet_r, bullet_r, edge_handle, 0, 1);
                    }
                } else {
                    bar_emit_filled_rect(t, (fastchart_obj *)self, x0, y0, x1, y1, color);
                    if (edge_handle >= 0) {
                        fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                    }
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
                bar_emit_filled_rect(t, (fastchart_obj *)self, x0, y0, x1, y1, color);
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
                bar_emit_filled_rect(t, (fastchart_obj *)self, x0, y0, x1, y1, color);
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
                if (y1 < y0) continue;

                if (self->bar_style == FASTCHART_BAR_STYLE_LOLLIPOP) {
                    /* Horizontal stem from the zero baseline to the value
                     * with a filled circle bullet at the value (vertical
                     * lollipop, X/Y swapped). */
                    int y_center = (y0 + y1) / 2;
                    int bullet_r = draw_h / 2;
                    if (bullet_r < 3) bullet_r = 3;
                    if (bullet_r > 12) bullet_r = 12;
                    fastchart_target_line(t, zero_x, y_center, x_v, y_center,
                                          color, 2, FASTCHART_DASH_SOLID);
                    fastchart_target_ellipse(t, x_v, y_center, bullet_r, bullet_r, color, 1, 0);
                    if (edge_handle >= 0) {
                        fastchart_target_ellipse(t, x_v, y_center, bullet_r, bullet_r, edge_handle, 0, 1);
                    }
                } else {
                    int x0 = x_v < zero_x ? x_v : zero_x;
                    int x1 = x_v < zero_x ? zero_x : x_v;
                    bar_emit_filled_rect(t, (fastchart_obj *)self, x0, y0, x1, y1, color);
                    if (edge_handle >= 0) {
                        fastchart_target_rect(t, x0, y0, x1 - x0 + 1, y1 - y0 + 1, edge_handle, 0, 1);
                    }
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
            fastchart_bar_slot g = fastchart_bar_slot_geom(plot.y0, plot.y1,
                i, n_categories, sub_count, bar_pct);
            int slot_top = g.left;
            int sub_h = g.sub;
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

    const char *legend_labels[FASTCHART_MAX_SERIES];
    for (int s = 0; s < n_series; s++) legend_labels[s] = series[s].label;
    fastchart_draw_series_legend(t, (fastchart_obj *)self, &plot, &pal,
                                 n_series, legend_labels);

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
            int py = fastchart_frac_to_px(frac_y, plot.y0, plot.y1);
            fastchart_blit_icon(t, ic, px, py);
        }
    }
    return 0;
}

/* Radial-bar (circular bar / "race track") render path. Categories
 * become concentric rings (category 0 outermost); each bar is a thick
 * arc whose angular length encodes the value, swept clockwise from 12
 * o'clock. The peak value across all series maps to a near-full circle
 * (a small gap keeps the ring start readable). Multiple series stack as
 * concentric sub-bands within each category's ring. A faint full-circle
 * track sits behind each bar so short bars read against the scale. */
static int fastchart_bar_render_radial(fastchart_bar_obj *self,
                                       fastchart_target_t *t)
{
    fastchart_series_t *series = self->series;
    int n_series = self->n_series;
    int n_categories = self->max_len;

    /* Peak value sets the angular full-scale. */
    double vmax = 0.0;
    for (int s = 0; s < n_series; s++) {
        for (int i = 0; i < series[s].len; i++) {
            double v = series[s].values[i];
            if (isfinite(v) && v > vmax) vmax = v;
        }
    }
    if (!(vmax > 0.0)) {
        zend_throw_error(NULL,
            "FastChart\\BarChart::draw(): radial mode requires at least one positive value");
        return -1;
    }

    fastchart_rect plot;
    fastchart_compute_layout((fastchart_obj *)self, t, 0, 0, NULL, 0, &plot);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    fastchart_obj *base = (fastchart_obj *)self;

    /* Reset image-map areas as the vertical (line 185) and horizontal
     * paths do. Radial bars are arcs with no rect hot-spots, so a reset
     * with no reserve/push is correct: it stops getImageMapAreas() from
     * returning stale rects left by a prior vertical/horizontal render
     * of the same object after setOrientation(BAR_RADIAL). */
    fastchart_reset_image_map_areas(base);

    fastchart_draw_frame(t, base, &plot, &pal);
    fastchart_draw_title(t, base, &plot, &pal);

    int cx = (plot.x0 + plot.x1) / 2;
    int cy = (plot.y0 + plot.y1) / 2;
    int avail_w = plot.x1 - plot.x0;
    int avail_h = plot.y1 - plot.y0;
    int max_r = (avail_w < avail_h ? avail_w : avail_h) / 2 - 12;
    if (max_r < 30) max_r = 30;
    int inner_r = max_r / 6;

    int cats = n_categories > 0 ? n_categories : 1;
    double bandw = (double)(max_r - inner_r) / (double)cats;
    int subc = n_series > 0 ? n_series : 1;
    double sub_thick = bandw / (double)subc;

    /* A peak bar sweeps 330 deg; the 30 deg gap keeps the 12-o'clock
     * start visible. Clockwise from -90 (SVG y grows downward, so a
     * positive sweep is clockwise on screen). */
    const double full_sweep = 330.0;
    const double start_deg = -90.0;

    int track = fastchart_target_color(t, 0xd8, 0xd4, 0xcc, 110);

    for (int c = 0; c < n_categories; c++) {
        double cat_outer = (double)max_r - (double)c * bandw;
        for (int s = 0; s < n_series; s++) {
            double v = (c < series[s].len) ? series[s].values[c] : NAN;
            if (!isfinite(v) || v < 0.0) v = 0.0;
            double arc_r = cat_outer - ((double)s + 0.5) * sub_thick;
            if (arc_r < 1.0) arc_r = 1.0;
            int thick = (int)(sub_thick * 0.7);
            if (thick < 1) thick = 1;
            int series_color = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            int color = bar_per_point_color(series[s].point_colors, c,
                                            series_color, t);

            fastchart_target_arc(t, cx, cy, (int)arc_r, (int)arc_r,
                                 start_deg, start_deg + 359.9,
                                 track, 0, thick);
            double sweep = full_sweep * (v / vmax);
            if (sweep > 0.5) {
                fastchart_target_arc(t, cx, cy, (int)arc_r, (int)arc_r,
                                     start_deg, start_deg + sweep,
                                     color, 0, thick);
            }
        }
    }

    /* Category labels just left of each ring's 12-o'clock start. */
    const char *font = fastchart_resolve_font(base, FC_FONT_LABEL);
    double fbase = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double fsize = fastchart_resolve_font_size(base, FC_FONT_LABEL, fbase);
    if (font && base->category_labels) {
        for (int c = 0; c < n_categories; c++) {
            const char *label = (c < base->n_category_labels)
                ? base->category_labels[c] : NULL;
            if (!label) continue;
            double cat_center = (double)max_r - ((double)c + 0.5) * bandw;
            int lx = cx - 6;
            int ly = cy - (int)cat_center + (int)(fsize * 0.35);
            fastchart_text_draw(t, font, fsize, pal.text,
                                lx, ly, FASTCHART_ALIGN_RIGHT, label, NULL, 0);
        }
    }

    const char *legend_labels[FASTCHART_MAX_SERIES];
    for (int s = 0; s < n_series; s++) legend_labels[s] = series[s].label;
    fastchart_draw_series_legend(t, base, &plot, &pal, n_series, legend_labels);

    fastchart_draw_text_annotations(t, base, &pal);
    return 0;
}
