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
#include <stdint.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

/* Force-directed network graph (Fruchterman-Reingold). The layout is
 * fully deterministic: initial node placement comes from a seeded
 * xorshift PRNG and the iteration count is fixed, so the same input +
 * seed always produces byte-identical output (Math.random is neither
 * available nor desirable here). O(n^2) repulsion per iteration, which
 * is fine for the node counts this chart targets. */

/* xorshift32 — seed must be non-zero. */
static inline uint32_t fc_xs_next(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}
static inline double fc_xs_unit(uint32_t *s)
{
    return (double)fc_xs_next(s) / (double)UINT32_MAX;
}

int fastchart_network_render_to_target(fastchart_network_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (self->node_count <= 0 || self->link_count <= 0) {
        zend_throw_error(NULL,
            "FastChart\\NetworkChart::draw() requires both setNodes() and setLinks()");
        return -1;
    }

    int top_pad = 16;
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

    int margin = 48;
    int plot_x0 = margin, plot_x1 = W - margin;
    int plot_y0 = top_pad + margin / 2, plot_y1 = H - margin;
    double plot_w = plot_x1 - plot_x0;
    double plot_h = plot_y1 - plot_y0;
    if (plot_w < 20.0 || plot_h < 20.0) return 0;

    int n = self->node_count;
    double *px = ecalloc(n, sizeof(*px));
    double *py = ecalloc(n, sizeof(*py));
    double *dx = ecalloc(n, sizeof(*dx));
    double *dy = ecalloc(n, sizeof(*dy));
    int *degree = ecalloc(n, sizeof(*degree));
    for (int e = 0; e < self->link_count; e++) {
        degree[self->links[e].from]++;
        degree[self->links[e].to]++;
    }

    uint32_t rng = (uint32_t)self->seed;
    if (rng == 0) rng = 1;
    for (int i = 0; i < n; i++) {
        px[i] = plot_x0 + fc_xs_unit(&rng) * plot_w;
        py[i] = plot_y0 + fc_xs_unit(&rng) * plot_h;
    }

    double area = plot_w * plot_h;
    double k = 0.8 * sqrt(area / (double)n);
    if (k < 1.0) k = 1.0;
    double k2 = k * k;                  /* loop-invariant; hoisted */
    int iters = (int)self->iterations;
    if (iters < 1) iters = 1;
    /* Repulsion is O(n^2) per pass, so total work is O(n^2 * iters).
     * Scale the iteration count down for large graphs so a single
     * render can't pin a CPU on adversarial input (n up to 512,
     * user-settable iters up to 5000). Small graphs keep their full
     * requested count. */
    long pairs = (long)n * n;
    long budget = 60000000L;           /* ~ pair-steps per render */
    if (pairs > 0 && (long)iters * pairs > budget) {
        iters = (int)(budget / pairs);
        if (iters < 1) iters = 1;
    }
    double temp0 = plot_w * 0.1;

    for (int it = 0; it < iters; it++) {
        for (int i = 0; i < n; i++) { dx[i] = 0.0; dy[i] = 0.0; }

        /* Repulsion between every pair. */
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                double ddx = px[i] - px[j];
                double ddy = py[i] - py[j];
                double dist = sqrt(ddx * ddx + ddy * ddy);
                if (dist < 0.01) {
                    /* Coincident: deterministic nudge from the PRNG. */
                    ddx = fc_xs_unit(&rng) - 0.5;
                    ddy = fc_xs_unit(&rng) - 0.5;
                    dist = sqrt(ddx * ddx + ddy * ddy) + 0.01;
                }
                double force = k2 / dist;
                double ux = ddx / dist, uy = ddy / dist;
                dx[i] += ux * force; dy[i] += uy * force;
                dx[j] -= ux * force; dy[j] -= uy * force;
            }
        }
        /* Attraction along edges. */
        for (int e = 0; e < self->link_count; e++) {
            int a = self->links[e].from, b = self->links[e].to;
            double ddx = px[a] - px[b];
            double ddy = py[a] - py[b];
            double dist = sqrt(ddx * ddx + ddy * ddy);
            if (dist < 0.01) dist = 0.01;
            double force = (dist * dist) / k;
            double ux = ddx / dist, uy = ddy / dist;
            dx[a] -= ux * force; dy[a] -= uy * force;
            dx[b] += ux * force; dy[b] += uy * force;
        }

        double temp = temp0 * (1.0 - (double)it / iters);
        if (temp < 0.5) temp = 0.5;
        for (int i = 0; i < n; i++) {
            double dlen = sqrt(dx[i] * dx[i] + dy[i] * dy[i]);
            if (dlen > 0.0) {
                double step = dlen < temp ? dlen : temp;
                px[i] += (dx[i] / dlen) * step;
                py[i] += (dy[i] / dlen) * step;
            }
            if (px[i] < plot_x0) px[i] = plot_x0;
            if (px[i] > plot_x1) px[i] = plot_x1;
            if (py[i] < plot_y0) py[i] = plot_y0;
            if (py[i] > plot_y1) py[i] = plot_y1;
        }
    }

    /* Rescale the final bounding box to fill the plot rect. */
    double minx = px[0], maxx = px[0], miny = py[0], maxy = py[0];
    for (int i = 1; i < n; i++) {
        if (px[i] < minx) minx = px[i];
        if (px[i] > maxx) maxx = px[i];
        if (py[i] < miny) miny = py[i];
        if (py[i] > maxy) maxy = py[i];
    }
    double spanx = maxx - minx, spany = maxy - miny;
    int pad = 24;
    for (int i = 0; i < n; i++) {
        px[i] = spanx > 1e-6
            ? plot_x0 + pad + (px[i] - minx) / spanx * (plot_w - 2 * pad)
            : (plot_x0 + plot_x1) / 2.0;
        py[i] = spany > 1e-6
            ? plot_y0 + pad + (py[i] - miny) / spany * (plot_h - 2 * pad)
            : (plot_y0 + plot_y1) / 2.0;
    }

    /* Edges first. */
    double max_val = 0.0;
    for (int e = 0; e < self->link_count; e++) {
        if (self->links[e].value > max_val) max_val = self->links[e].value;
    }
    if (max_val <= 0.0) max_val = 1.0;
    for (int e = 0; e < self->link_count; e++) {
        const fastchart_graph_link *lk = &self->links[e];
        int thickness = (int)(1.0 + (lk->value / max_val) * 3.0);
        if (thickness < 1) thickness = 1;
        fastchart_target_line(t, (int)px[lk->from], (int)py[lk->from],
                              (int)px[lk->to], (int)py[lk->to],
                              pal.border, thickness, FASTCHART_DASH_SOLID);
    }

    /* Nodes + labels. */
    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double size = fastchart_resolve_font_size(
        (fastchart_obj *)self, FC_FONT_LABEL, base_size);
    int max_deg = 1;
    for (int i = 0; i < n; i++) if (degree[i] > max_deg) max_deg = degree[i];
    for (int i = 0; i < n; i++) {
        int color = self->nodes[i].color_rgb >= 0
            ? fastchart_target_color_rgb(t, self->nodes[i].color_rgb)
            : pal.series[i % FASTCHART_PALETTE_SERIES_N];
        int r = (int)(4.0 + 6.0 * sqrt((double)degree[i] / max_deg));
        if (r < 4) r = 4;
        fastchart_target_ellipse(t, (int)px[i], (int)py[i], r, r, color, 1, 0);
        fastchart_target_ellipse(t, (int)px[i], (int)py[i], r, r, pal.border, 0, 1);
        if (font && self->nodes[i].label) {
            fastchart_text_draw(t, font, size, pal.text,
                                (int)px[i], (int)(py[i] - r - 2),
                                FASTCHART_ALIGN_CENTER,
                                self->nodes[i].label, NULL, 0);
        }
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    efree(px); efree(py); efree(dx); efree(dy); efree(degree);
    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
