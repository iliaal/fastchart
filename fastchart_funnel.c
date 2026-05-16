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

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

int fastchart_funnel_render_to_target(fastchart_funnel_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    /* Title reservation. */
    int top_pad = 12;
    int title_h = 0;
    const char *title_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_TITLE);
    double base_size = self->font_size > 0 ? self->font_size : FASTCHART_DEFAULT_FONT_SIZE;
    double title_size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_TITLE, base_size * 1.4);
    if (self->title && ZSTR_LEN(self->title) > 0 && title_font) {
        if (fastchart_text_measure(t, title_font, title_size, ZSTR_VAL(self->title),
                                   NULL, &title_h, NULL, 0) == 0) {
            top_pad += title_h + 10;
        }
    }

    int n = self->stage_count;
    if (n <= 0) {
        zend_throw_error(NULL,
            "FastChart\\Funnel::draw() requires setStages() with at least one stage");
        return -1;
    }

    /* Plot region: full width minus side padding for stage labels,
     * vertical span from below the title to the canvas bottom minus
     * a small footer. */
    int side_pad = 100;     /* room for label text on either side */
    int x_left = side_pad;
    int x_right = W - side_pad;
    int y0 = top_pad;
    int y1 = H - 12;
    if (x_right <= x_left || y1 <= y0) {
        zend_throw_error(NULL,
            "FastChart\\Funnel::draw() canvas is too narrow for label margins");
        return -1;
    }

    /* Per-stage layout differs by style. FUNNEL: equal-height
     * trapezoids; widths track each stage's value relative to the
     * largest stage. PYRAMID: single triangle subdivided into
     * value-proportional bands; widths follow the triangle's
     * natural taper (0 at the apex, full at the base). */
    double max_v = 0, total_v = 0;
    for (int i = 0; i < n; i++) {
        if (self->stages[i].value > max_v) max_v = self->stages[i].value;
        total_v += self->stages[i].value;
    }
    if (max_v <= 0 || total_v <= 0) {
        zend_throw_error(NULL,
            "FastChart\\Funnel::draw() requires at least one stage with value > 0");
        return -1;
    }

    int cx = (x_left + x_right) / 2;
    int max_half = (x_right - x_left) / 2;

    bool pyramid = (self->funnel_style == FASTCHART_FUNNEL_STYLE_PYRAMID);
    bool cone    = (self->funnel_style == FASTCHART_FUNNEL_STYLE_CONE);
    /* CONE bands draw front-facing ellipse arcs at the top and
     * bottom of every band. The bottom arc dips below the band's
     * geometric y by ring_h ≈ 22% of the band's average half-width.
     * For the bottom-most band that half is max_half (the widest
     * point), so reserve max_half * 0.22 pixels at the bottom of
     * the plot region — otherwise the bottom band's arc spills
     * past the canvas edge. */
    int cone_bottom_reserve = cone ? (int)(max_half * 0.22 + 0.5) : 0;
    int total_h = (y1 - y0) - cone_bottom_reserve;
    int stage_h = total_h / n;
    if (stage_h < 4) stage_h = 4;
    /* CONE shares PYRAMID's triangular layout (cumulative-value y
     * positions, value-proportional band heights, linear width
     * taper) but renders each band's top and bottom edges as front-
     * facing ellipse arcs to suggest a 3D cone seen from the side.
     * Treat cone like pyramid for layout decisions, then swap the
     * polygon shape at draw time. */
    if (cone) pyramid = true;

    const char *label_font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    double label_size = fastchart_resolve_font_size((fastchart_obj *)self, FC_FONT_LABEL, base_size);

    /* PYRAMID: cumulative-value scan top-down to position each band
     * within the triangle. Width at any y is linear in (y - y0). */
    double cum_v = 0.0;
    for (int i = 0; i < n; i++) {
        double v_top, v_bot;
        int half_top, half_bot, yt, yb;
        if (pyramid) {
            double y_top = y0 + cum_v / total_v * total_h;
            cum_v += self->stages[i].value;
            double y_bot = y0 + cum_v / total_v * total_h;
            yt = (int)(y_top + 0.5);
            yb = (int)(y_bot + 0.5) - 1;
            if (yb <= yt) yb = yt + 1;
            half_top = (int)(max_half * (y_top - y0) / total_h + 0.5);
            half_bot = (int)(max_half * (y_bot - y0) / total_h + 0.5);
            v_top = self->stages[i].value;
            v_bot = v_top;  /* unused in pyramid label path; kept for symmetry */
        } else {
            v_top = self->stages[i].value;
            v_bot = (i + 1 < n) ? self->stages[i + 1].value : v_top * 0.6;
            if (v_bot < 0) v_bot = 0;
            half_top = (int)(max_half * v_top / max_v + 0.5);
            half_bot = (int)(max_half * v_bot / max_v + 0.5);
            yt = y0 + i * stage_h;
            yb = yt + stage_h - 2;
        }

        int color;
        if (self->stages[i].color_rgb >= 0) {
            color = fastchart_target_color_rgb(t, self->stages[i].color_rgb);
        } else {
            color = pal.series[i % FASTCHART_PALETTE_SERIES_N];
        }

        if (cone) {
            /* Trace: top arc (left→right, dipping below yt), down
             * right wall, bottom arc (right→left, dipping below yb),
             * up left wall. ring_h sets the apparent perspective
             * tilt — 25% of the band's average half-width gives a
             * moderate camera angle. ARC_N=14 samples per arc keeps
             * the silhouette smooth without blowing the polygon
             * point budget. */
            enum { ARC_N = 14, CONE_PTS = (ARC_N + 1) * 2 };
            double avg_half = (half_top + half_bot) * 0.5;
            int ring_h_top = (int)(avg_half * 0.22 + 0.5);
            int ring_h_bot = (int)(avg_half * 0.22 + 0.5);
            if (ring_h_top < 1) ring_h_top = 1;
            if (ring_h_bot < 1) ring_h_bot = 1;
            gdPoint band[CONE_PTS];
            int k = 0;
            /* Top arc: θ from π (left) → 0 (right), front-facing
             * (sin >= 0 dips below yt). */
            for (int s = 0; s <= ARC_N; s++) {
                double th = M_PI - (double)s * M_PI / ARC_N;
                band[k].x = cx + (int)(half_top * cos(th) + 0.5);
                band[k].y = yt + (int)(ring_h_top * sin(th) + 0.5);
                k++;
            }
            /* Bottom arc: θ from 0 (right) → π (left). */
            for (int s = 0; s <= ARC_N; s++) {
                double th = (double)s * M_PI / ARC_N;
                band[k].x = cx + (int)(half_bot * cos(th) + 0.5);
                band[k].y = yb + (int)(ring_h_bot * sin(th) + 0.5);
                k++;
            }
            fastchart_target_polygon(t, band, k, color, 1, 0);
            fastchart_target_polygon(t, band, k, pal.border, 0, 1);
        } else {
            gdPoint trap[4] = {
                { cx - half_top, yt },
                { cx + half_top, yt },
                { cx + half_bot, yb },
                { cx - half_bot, yb },
            };
            fastchart_target_polygon(t, trap, 4, color, 1, 0);
            fastchart_target_polygon(t, trap, 4, pal.border, 0, 1);
        }

        /* Label: stage name on the left, optional value on the right.
         * Anchor at the trapezoid centroid Y. */
        int yc = (yt + yb) / 2 + (int)(label_size * 0.4);
        if (label_font && self->stages[i].label) {
            fastchart_text_draw(t, label_font, label_size, pal.text,
                                x_left - 8, yc, FASTCHART_ALIGN_RIGHT,
                                self->stages[i].label, NULL, 0);
        }
        if (label_font && ((fastchart_obj *)self)->show_values) {
            /* Honour the inherited setShowValues($flag, $format)
             * format string when present; fall back to "%.0f" for
             * the unconfigured case. */
            const char *fmt = "%.0f";
            fastchart_obj *base = (fastchart_obj *)self;
            if (base->value_format && ZSTR_LEN(base->value_format) > 0) {
                fmt = ZSTR_VAL(base->value_format);
            }
            char buf[64];
            snprintf(buf, sizeof(buf), fmt, v_top);
            fastchart_text_draw(t, label_font, label_size, pal.text,
                                x_right + 8, yc, FASTCHART_ALIGN_LEFT,
                                buf, NULL, 0);
        }
    }

    /* Title. */
    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}

