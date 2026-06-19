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

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"

/* Read a value from a typed series at the given index. Returns NaN
 * if the index is past the series end or the cell is a gap. */
static inline double area_read_value(const fastchart_series_t *s, int i)
{
    if (i >= s->len) return NAN;
    return s->values[i];
}

/* Densify an area layer's top boundary to follow the chart's
 * line_interpolation, so a smooth or stepped area traces the same
 * curve as the equivalent line. raw[] holds the n per-category
 * vertices already in pixel space (area folds gaps to 0, so every
 * category is a vertex); writes at most cap points into out[] and
 * returns the count. The shapes mirror the line renderer's
 * polyline_pass: a right-angle corner per segment for the STEP_*
 * modes, Catmull-Rom sampling for SMOOTH. Callers invoke it only for
 * the non-linear modes; LINEAR keeps the original vertex-for-vertex
 * fill path untouched. */
static int area_densify(int interp, const gdPoint *raw, int n,
                        gdPoint *out, int cap)
{
    int m = 0;
    if (n <= 0 || cap <= 0) return 0;
    out[m++] = raw[0];
    if (interp == FASTCHART_INTERP_STEP_AFTER ||
        interp == FASTCHART_INTERP_STEP_BEFORE) {
        bool after = (interp == FASTCHART_INTERP_STEP_AFTER);
        for (int i = 1; i < n && m + 2 <= cap; i++) {
            gdPoint corner;
            if (after) { corner.x = raw[i].x;     corner.y = raw[i - 1].y; }
            else       { corner.x = raw[i - 1].x; corner.y = raw[i].y;     }
            out[m++] = corner;
            out[m++] = raw[i];
        }
        return m;
    }
    /* SMOOTH: Catmull-Rom with the same adaptive sub-segment count as
     * the line renderer (~one sub-segment per 4 px, clamped to 2..20). */
    for (int i = 0; i < n - 1 && m < cap; i++) {
        int p0i = (i > 0) ? i - 1 : i;
        int p3i = (i + 2 < n) ? i + 2 : i + 1;
        int dx = raw[i + 1].x - raw[i].x;
        int dy = raw[i + 1].y - raw[i].y;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        int subdiv = (dx + dy) / 4;
        if (subdiv < 2)  subdiv = 2;
        if (subdiv > 20) subdiv = 20;
        for (int k = 1; k <= subdiv && m < cap; k++) {
            double tt = (double)k / (double)subdiv;
            int x, y;
            fastchart_catmull_point(raw[p0i].x, raw[p0i].y,
                                    raw[i].x, raw[i].y,
                                    raw[i + 1].x, raw[i + 1].y,
                                    raw[p3i].x, raw[p3i].y,
                                    tt, &x, &y);
            out[m].x = x; out[m].y = y; m++;
        }
    }
    return m;
}

int fastchart_area_render_to_target(fastchart_area_obj *self, fastchart_target_t *t)
{
    if (self->n_series == 0) {
        zend_throw_error(NULL,
            "FastChart\\AreaChart::draw() requires setSeries() to have been called with non-empty data");
        return -1;
    }
    fastchart_series_t *series = self->series;
    int n_series = self->n_series;
    int max_len = self->max_len;

    bool stacked = self->stacked;
    if (n_series < 2) stacked = false;
    /* Stream graph (ThemeRiver): a stacked area centered on a baseline
     * instead of anchored at zero. It needs >=2 series, is always a
     * stacked variant, and ignores the secondary axis (all series stack
     * on the left). Assumes non-negative data; negatives are clamped to
     * 0 for the centering total. */
    bool stream = self->stream_mode && n_series >= 2;
    if (stream) stacked = true;
    /* Band mode requires exactly two series — the polygon is the
     * envelope between series[0] (upper) and series[1] (lower).
     * Falls back to the regular fill-to-baseline path if the caller
     * set band_mode without two series, or stacked is also set
     * (stacked wins, band is silently ignored). */
    bool band = self->band_mode && n_series == 2 && !stacked;

    /* With secondary_y off, all series map to the left axis regardless
     * of the per-series right_axis flag — mirrors LineChart. */
    int n_right = 0;
    if (self->secondary_y && !stream) {
        for (int s = 0; s < n_series; s++) {
            if (series[s].right_axis) n_right++;
        }
    }

    double dmin_l = 0, dmax_l = 0, dmin_r = 0, dmax_r = 0;
    int seen_l = 0, seen_r = 0;

    if (stacked) {
        /* The layer polygons span [cum, cum + v] (see the draw loop
         * below), so the axis range must cover every partial
         * cumulative sum — not just the per-category total. Negative
         * values pull partials below zero; folding only the final sum
         * would clamp those layers onto the baseline. Left- and
         * right-axis series stack independently when secondary_y is on. */
        for (int i = 0; i < max_len; i++) {
            double cum_l = 0, cum_r = 0;
            for (int s = 0; s < n_series; s++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) continue;
                bool right = self->secondary_y && !stream && series[s].right_axis;
                if (right) {
                    cum_r += v;
                    if (!seen_r) { dmin_r = dmax_r = cum_r; seen_r = 1; }
                    else {
                        if (cum_r < dmin_r) dmin_r = cum_r;
                        if (cum_r > dmax_r) dmax_r = cum_r;
                    }
                } else {
                    cum_l += v;
                    if (!seen_l) { dmin_l = dmax_l = cum_l; seen_l = 1; }
                    else {
                        if (cum_l < dmin_l) dmin_l = cum_l;
                        if (cum_l > dmax_l) dmax_l = cum_l;
                    }
                }
            }
        }
        if (self->y_axis_scale != FASTCHART_SCALE_LOG) {
            if (dmin_l > 0) dmin_l = 0;
            if (dmax_l < 0) dmax_l = 0;
            if (n_right > 0) {
                if (dmin_r > 0) dmin_r = 0;
                if (dmax_r < 0) dmax_r = 0;
            }
        }
    } else {
        for (int s = 0; s < n_series; s++) {
            bool right = self->secondary_y && series[s].right_axis;
            for (int i = 0; i < series[s].len; i++) {
                double d = series[s].values[i];
                if (isnan(d)) continue;
                if (right) {
                    if (!seen_r) { dmin_r = dmax_r = d; seen_r = 1; }
                    else { if (d < dmin_r) dmin_r = d; if (d > dmax_r) dmax_r = d; }
                } else {
                    if (!seen_l) { dmin_l = dmax_l = d; seen_l = 1; }
                    else { if (d < dmin_l) dmin_l = d; if (d > dmax_l) dmax_l = d; }
                }
            }
        }
        /* Zero-anchor the fill on linear scale only: a log axis has
         * no zero, so the fill anchors at the axis bottom instead
         * (the documented "Y-axis min, whichever is higher"). Both
         * signs need the anchor: an all-negative range would exclude
         * 0, and the fill baseline would clamp to the plot top (the
         * axis max), inverting the fill. */
        if (self->y_axis_scale != FASTCHART_SCALE_LOG) {
            if (dmin_l > 0) dmin_l = 0;
            if (dmax_l < 0) dmax_l = 0;
            if (n_right > 0) {
                if (dmin_r > 0) dmin_r = 0;
                if (dmax_r < 0) dmax_r = 0;
            }
        }
    }
    if (stream) {
        /* Center the stack on zero: the symmetric range is [-T/2, +T/2]
         * where T is the tallest per-category total. */
        double max_total = 0.0;
        for (int i = 0; i < max_len; i++) {
            double tot = 0.0;
            for (int s = 0; s < n_series; s++) {
                double v = area_read_value(&series[s], i);
                if (!isnan(v) && v > 0.0) tot += v;
            }
            if (tot > max_total) max_total = tot;
        }
        if (max_total <= 0.0) max_total = 1.0;
        dmin_l = -max_total / 2.0;
        dmax_l = max_total / 2.0;
        seen_l = 1;
    }
    if (!seen_l) {
        zend_throw_error(NULL,
            "FastChart\\AreaChart::draw() found no numeric values for the primary Y axis");
        return -1;
    }
    if (n_right > 0 && !seen_r) {
        zend_throw_error(NULL,
            "FastChart\\AreaChart::draw() found no numeric values for the secondary Y axis");
        return -1;
    }

    fastchart_value_range range_l, range_r;
    if (self->y_axis_scale == FASTCHART_SCALE_LOG) {
        if (stacked) {
            zend_value_error("FastChart\\AreaChart::draw(): log Y-axis requires non-stacked data (stacked areas anchor at 0)");
            return -1;
        }
        if (dmin_l <= 0) {
            zend_value_error("FastChart\\AreaChart::draw(): log Y-axis requires strictly-positive data");
            return -1;
        }
        if (fastchart_value_range_compute_log(dmin_l, dmax_l, &range_l) != 0) {
            zend_value_error("FastChart\\AreaChart::draw(): log Y-axis requires strictly-positive data");
            return -1;
        }
    } else {
        fastchart_value_range_compute(dmin_l, dmax_l, 6, &range_l);
        fastchart_value_range_apply_override((fastchart_obj *)self, &range_l);
    }
    if (n_right > 0) {
        fastchart_value_range_compute(dmin_r, dmax_r, 6, &range_r);
    }

    fastchart_rect plot;
    fastchart_palette pal;
    fastchart_render_cartesian_setup((fastchart_obj *)self, t,
                                     1, 1, NULL, 0, &plot, &pal);

    fastchart_draw_y_axis(t, (fastchart_obj *)self, &plot, &pal, &range_l);
    if (n_right > 0) {
        fastchart_draw_y_axis_right(t, (fastchart_obj *)self, &plot, &pal, &range_r);
    }
    fastchart_draw_plot_bands(t, (fastchart_obj *)self, &plot, &range_l, &pal);
    fastchart_draw_v_plot_bands_categorical(t, (fastchart_obj *)self, &plot,
                                            max_len, &pal);

    const char **label_ptrs = fastchart_borrow_category_labels((fastchart_obj *)self, max_len);
    fastchart_draw_x_axis_categorical(t, (fastchart_obj *)self, &plot, &pal, max_len, label_ptrs);
    if (label_ptrs) efree((void *)label_ptrs);

    fastchart_draw_axis_titles(t, (fastchart_obj *)self, &plot, &pal);

    int alpha = (int)self->area_alpha;
    if (alpha < 0) alpha = 0;
    if (alpha > 127) alpha = 127;

    int edge_handle = self->edge_color >= 0
        ? fastchart_target_color_rgb(t, (int)self->edge_color) : -1;

    /* Smooth / stepped fills follow the line interpolation. Band and
     * stream are special envelope shapes; they stay linear. */
    int interp = (int)self->line_interpolation;
    bool curve = !band && !stream &&
                 (interp == FASTCHART_INTERP_SMOOTH ||
                  interp == FASTCHART_INTERP_STEP_AFTER ||
                  interp == FASTCHART_INTERP_STEP_BEFORE);

    /* Build filled polygons. For stacked, accumulate per-category
     * sums; each series's polygon spans [prev_cum, prev_cum + v].
     * For non-stacked overlay, each series's polygon spans
     * [0, v] with translucent fill so layered shapes show through. */
    gdPoint poly[2 * FASTCHART_MAX_POINTS_PER_SERIES];

    /* Stream centering offset per category: -total/2, applied to every
     * layer so the stack straddles the zero line. NULL (and a 0 offset)
     * for the plain stacked / non-stacked paths, which stay unchanged. */
    double *stream_off = NULL;
    if (stream) {
        stream_off = ecalloc((size_t)max_len, sizeof(double));
        for (int i = 0; i < max_len; i++) {
            double tot = 0.0;
            for (int s = 0; s < n_series; s++) {
                double v = area_read_value(&series[s], i);
                if (!isnan(v) && v > 0.0) tot += v;
            }
            stream_off[i] = -tot / 2.0;
        }
    }

    if (band) {
        /* Build one closed polygon: series[0] left→right along the
         * top, then series[1] right→left along the bottom. The fill
         * is opaque (no per-series alpha layering here, the band
         * IS one shape). */
        int n_pts = 0;
        for (int i = 0; i < max_len && n_pts < 2048; i++) {
            double v = area_read_value(&series[0], i);
            if (isnan(v)) v = 0;
            int x = fastchart_x_categorical_center(&plot, i, max_len);
            int y = fastchart_y_to_pixel(v, &range_l, &plot);
            poly[n_pts].x = x; poly[n_pts].y = y;
            n_pts++;
        }
        for (int i = max_len - 1; i >= 0 && n_pts < 2 * 2048; i--) {
            double v = area_read_value(&series[1], i);
            if (isnan(v)) v = 0;
            int x = fastchart_x_categorical_center(&plot, i, max_len);
            int y = fastchart_y_to_pixel(v, &range_l, &plot);
            poly[n_pts].x = x; poly[n_pts].y = y;
            n_pts++;
        }
        int fill_handle = pal.series[0];
        uint32_t rgba = fastchart_target_color_to_rgba(t, fill_handle);
        int r = (rgba >> 16) & 0xFF;
        int g = (rgba >>  8) & 0xFF;
        int b =  rgba        & 0xFF;
        int alpha_byte = 255 - alpha * 2;
        if (alpha_byte < 0) alpha_byte = 0;
        int alpha_handle = fastchart_target_color(t, r, g, b, alpha_byte);
        if (n_pts >= 3) {
            fastchart_obj *base = (fastchart_obj *)self;
            if (base->gradient_from >= 0 && base->gradient_to >= 0) {
                uint32_t a = (uint32_t)alpha_byte & 0xFFu;
                uint32_t grad_from =
                    (a << 24) | ((uint32_t)base->gradient_from & 0xFFFFFFu);
                uint32_t grad_to =
                    (a << 24) | ((uint32_t)base->gradient_to   & 0xFFFFFFu);
                fastchart_target_gradient_polygon(t, poly, n_pts,
                    grad_from, grad_to, (int)base->gradient_dir);
            } else {
                fastchart_target_polygon(t, poly, n_pts, alpha_handle, 1, 0);
            }
            if (edge_handle >= 0) {
                fastchart_target_polygon(t, poly, n_pts, edge_handle, 0, 1);
            }
        }
        /* Stroke the two boundary curves so the band has crisp
         * upper and lower edges regardless of fill alpha. */
        for (int s = 0; s < 2; s++) {
            int stroke_handle = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            int prev_x = 0, prev_y = 0;
            bool prev_valid = false;
            for (int i = 0; i < max_len; i++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) { prev_valid = false; continue; }
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(v, &range_l, &plot);
                if (prev_valid) {
                    fastchart_target_line(t, prev_x, prev_y, x, y,
                                          stroke_handle, 2, FASTCHART_DASH_SOLID);
                }
                prev_x = x; prev_y = y; prev_valid = true;
            }
        }
    } else if (stacked) {
        double *cum_l = ecalloc((size_t)max_len, sizeof(double));
        double *cum_r = n_right > 0 ? ecalloc((size_t)max_len, sizeof(double)) : NULL;
        for (int s = 0; s < n_series; s++) {
            bool right = self->secondary_y && !stream && series[s].right_axis;
            double *cum = right ? cum_r : cum_l;
            const fastchart_value_range *rng = right ? &range_r : &range_l;
            int series_handle = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            int n_pts = 0;

            if (curve) {
                /* Curved layer: densify both the top (cum + v) and the
                 * bottom (cum) boundaries with the same interpolation so
                 * adjacent layers tile without gaps. stream_off is 0
                 * here — stream excludes the curve path. */
                int vcap = max_len * 21 + 8;
                gdPoint *rawt = emalloc((size_t)max_len * sizeof(gdPoint));
                gdPoint *rawb = emalloc((size_t)max_len * sizeof(gdPoint));
                gdPoint *dt = emalloc((size_t)vcap * sizeof(gdPoint));
                gdPoint *db = emalloc((size_t)vcap * sizeof(gdPoint));
                for (int i = 0; i < max_len; i++) {
                    double v = area_read_value(&series[s], i);
                    if (isnan(v)) v = 0;
                    int x = fastchart_x_categorical_center(&plot, i, max_len);
                    rawt[i].x = x;
                    rawt[i].y = fastchart_y_to_pixel(cum[i] + v, rng, &plot);
                    rawb[i].x = x;
                    rawb[i].y = fastchart_y_to_pixel(cum[i], rng, &plot);
                }
                int dnt = area_densify(interp, rawt, max_len, dt, vcap);
                int dnb = area_densify(interp, rawb, max_len, db, vcap);
                if (dnt >= 2 && dnb >= 2) {
                    int fcap = dnt + dnb;
                    gdPoint *fp = emalloc((size_t)fcap * sizeof(gdPoint));
                    int fn = 0;
                    for (int i = 0; i < dnt; i++) fp[fn++] = dt[i];
                    for (int i = dnb - 1; i >= 0; i--) fp[fn++] = db[i];
                    fastchart_obj *base = (fastchart_obj *)self;
                    if (base->gradient_from >= 0 && base->gradient_to >= 0) {
                        fastchart_target_gradient_polygon(t, fp, fn,
                            (uint32_t)base->gradient_from,
                            (uint32_t)base->gradient_to,
                            (int)base->gradient_dir);
                    } else {
                        fastchart_target_polygon(t, fp, fn, series_handle, 1, 0);
                    }
                    if (edge_handle >= 0) {
                        fastchart_target_polygon(t, fp, fn, edge_handle, 0, 1);
                    }
                    efree(fp);
                }
                for (int i = 1; i < dnt; i++) {
                    fastchart_target_line(t, dt[i - 1].x, dt[i - 1].y,
                                          dt[i].x, dt[i].y, pal.border, 1,
                                          FASTCHART_DASH_SOLID);
                }
                for (int i = 0; i < max_len; i++) {
                    double v = area_read_value(&series[s], i);
                    if (isnan(v)) v = 0;
                    cum[i] += v;
                }
                efree(rawt); efree(rawb); efree(dt); efree(db);
                continue;
            }

            /* Top edge: left to right at cum + v (+ stream centering). */
            for (int i = 0; i < max_len && n_pts < 2048; i++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) v = 0;
                double soff = stream_off ? stream_off[i] : 0.0;
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(cum[i] + v + soff, rng, &plot);
                poly[n_pts].x = x; poly[n_pts].y = y;
                n_pts++;
            }
            /* Bottom edge: right to left at cum (+ stream centering). */
            for (int i = max_len - 1; i >= 0 && n_pts < 2 * 2048; i--) {
                double soff = stream_off ? stream_off[i] : 0.0;
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(cum[i] + soff, rng, &plot);
                poly[n_pts].x = x; poly[n_pts].y = y;
                n_pts++;
            }
            if (n_pts >= 3) {
                fastchart_obj *base = (fastchart_obj *)self;
                if (base->gradient_from >= 0 && base->gradient_to >= 0) {
                    fastchart_target_gradient_polygon(t, poly, n_pts,
                        (uint32_t)base->gradient_from,
                        (uint32_t)base->gradient_to,
                        (int)base->gradient_dir);
                } else {
                    fastchart_target_polygon(t, poly, n_pts, series_handle, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_polygon(t, poly, n_pts, edge_handle, 0, 1);
                }
            }

            /* Top-edge stroke for crisp boundary between layers. */
            int prev_x = 0, prev_y = 0;
            bool prev_valid = false;
            for (int i = 0; i < max_len; i++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) v = 0;
                double soff = stream_off ? stream_off[i] : 0.0;
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(cum[i] + v + soff, rng, &plot);
                if (prev_valid) {
                    fastchart_target_line(t, prev_x, prev_y, x, y,
                                          pal.border, 1, FASTCHART_DASH_SOLID);
                }
                prev_x = x; prev_y = y; prev_valid = true;
                cum[i] += v;
            }
        }
        efree(cum_l);
        if (cum_r) efree(cum_r);
    } else {
        for (int s = 0; s < n_series; s++) {
            bool right = self->secondary_y && series[s].right_axis;
            const fastchart_value_range *rng = right ? &range_r : &range_l;
            /* Fill anchor: the zero baseline on linear scale, the axis
             * bottom on log (zero is not representable there). */
            int zero_y = fastchart_y_to_pixel(
                rng->log_scale ? rng->min : 0.0, rng, &plot);

            int base_handle = pal.series[s % FASTCHART_PALETTE_SERIES_N];
            uint32_t rgba = fastchart_target_color_to_rgba(t, base_handle);
            int r = (rgba >> 16) & 0xFF;
            int g = (rgba >>  8) & 0xFF;
            int b =  rgba        & 0xFF;
            /* gd alpha 0..127 -> byte 255..1 via 255 - gd_alpha * 2. */
            int alpha_byte = 255 - alpha * 2;
            if (alpha_byte < 0) alpha_byte = 0;
            int alpha_handle = fastchart_target_color(t, r, g, b, alpha_byte);

            if (curve) {
                /* Curved fill: densified top edge, then a straight
                 * baseline back to close the polygon. */
                int vcap = max_len * 21 + 8;
                gdPoint *raw = emalloc((size_t)max_len * sizeof(gdPoint));
                gdPoint *dens = emalloc((size_t)vcap * sizeof(gdPoint));
                for (int i = 0; i < max_len; i++) {
                    double v = area_read_value(&series[s], i);
                    if (isnan(v)) v = 0;
                    raw[i].x = fastchart_x_categorical_center(&plot, i, max_len);
                    raw[i].y = fastchart_y_to_pixel(v, rng, &plot);
                }
                int dn = area_densify(interp, raw, max_len, dens, vcap);
                if (dn >= 2) {
                    int fcap = dn + 2;
                    gdPoint *fp = emalloc((size_t)fcap * sizeof(gdPoint));
                    int fn = 0;
                    for (int i = 0; i < dn; i++) fp[fn++] = dens[i];
                    fp[fn].x = dens[dn - 1].x; fp[fn].y = zero_y; fn++;
                    fp[fn].x = dens[0].x;      fp[fn].y = zero_y; fn++;
                    fastchart_obj *base = (fastchart_obj *)self;
                    if (base->gradient_from >= 0 && base->gradient_to >= 0) {
                        uint32_t a = (uint32_t)alpha_byte & 0xFFu;
                        uint32_t grad_from =
                            (a << 24) | ((uint32_t)base->gradient_from & 0xFFFFFFu);
                        uint32_t grad_to =
                            (a << 24) | ((uint32_t)base->gradient_to   & 0xFFFFFFu);
                        fastchart_target_gradient_polygon(t, fp, fn,
                            grad_from, grad_to, (int)base->gradient_dir);
                    } else {
                        fastchart_target_polygon(t, fp, fn, alpha_handle, 1, 0);
                    }
                    if (edge_handle >= 0) {
                        fastchart_target_polygon(t, fp, fn, edge_handle, 0, 1);
                    }
                    efree(fp);
                }
                for (int i = 1; i < dn; i++) {
                    fastchart_target_line(t, dens[i - 1].x, dens[i - 1].y,
                                          dens[i].x, dens[i].y, base_handle, 2,
                                          FASTCHART_DASH_SOLID);
                }
                efree(raw); efree(dens);
                continue;
            }

            int n_pts = 0;
            for (int i = 0; i < max_len && n_pts < 2048; i++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) v = 0;
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(v, rng, &plot);
                poly[n_pts].x = x; poly[n_pts].y = y;
                n_pts++;
            }
            for (int i = max_len - 1; i >= 0 && n_pts < 2 * 2048; i--) {
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                poly[n_pts].x = x; poly[n_pts].y = zero_y;
                n_pts++;
            }
            if (n_pts >= 3) {
                fastchart_obj *base = (fastchart_obj *)self;
                if (base->gradient_from >= 0 && base->gradient_to >= 0) {
                    /* Compose setAreaAlpha (0..127 gd convention,
                     * 127 = transparent) into the gradient stops so
                     * non-stacked overlay layers stay translucent
                     * the way solid-fill overlays do. */
                    uint32_t a = (uint32_t)alpha_byte & 0xFFu;
                    uint32_t grad_from =
                        (a << 24) | ((uint32_t)base->gradient_from & 0xFFFFFFu);
                    uint32_t grad_to =
                        (a << 24) | ((uint32_t)base->gradient_to   & 0xFFFFFFu);
                    fastchart_target_gradient_polygon(t, poly, n_pts,
                        grad_from, grad_to, (int)base->gradient_dir);
                } else {
                    fastchart_target_polygon(t, poly, n_pts, alpha_handle, 1, 0);
                }
                if (edge_handle >= 0) {
                    fastchart_target_polygon(t, poly, n_pts, edge_handle, 0, 1);
                }
            }

            /* Opaque top stroke. */
            int prev_x = 0, prev_y = 0;
            bool prev_valid = false;
            for (int i = 0; i < max_len; i++) {
                double v = area_read_value(&series[s], i);
                if (isnan(v)) { prev_valid = false; continue; }
                int x = fastchart_x_categorical_center(&plot, i, max_len);
                int y = fastchart_y_to_pixel(v, rng, &plot);
                if (prev_valid) {
                    fastchart_target_line(t, prev_x, prev_y, x, y,
                                          base_handle, 2, FASTCHART_DASH_SOLID);
                }
                prev_x = x; prev_y = y; prev_valid = true;
            }
        }
    }

    if (stream_off) efree(stream_off);

    /* Combo overlays + annotations on top of the area fills. */
    fastchart_draw_overlays_categorical(t, (fastchart_obj *)self, &plot, &pal,
                                         &range_l,
                                         n_right > 0 ? &range_r : NULL,
                                         max_len);

    fastchart_draw_h_annotations(t, (fastchart_obj *)self, &plot, &pal, &range_l);
    fastchart_draw_v_annotations_categorical(t, (fastchart_obj *)self, &plot, &pal, max_len);

    /* Legend. */
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

    if (self->icons && self->n_icons > 0 && max_len > 0) {
        for (int i = 0; i < self->n_icons; i++) {
            const fastchart_icon *ic = &self->icons[i];
            double frac_x = max_len > 1
                ? (ic->x + 0.5) / (double)max_len
                : 0.5;
            int px = fastchart_frac_to_px(frac_x, plot.x0, plot.x1);
            int py = fastchart_y_to_pixel(ic->y, &range_l, &plot);
            fastchart_blit_icon(t, ic, px, py);
        }
    }
    return 0;
}

