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

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_palette.h"
#include "fastchart_target.h"
#include "fastchart_axis.h"
#include "fastchart_text.h"

/* Word cloud: each word's font size scales with its weight; words are
 * placed largest-first along an Archimedean spiral from the centre,
 * skipping any position whose bounding box collides with an already-
 * placed word. Placement is deterministic (sorted input + fixed spiral).
 * Words are drawn horizontally — mixed orientation is intentionally not
 * attempted so the rotated-bbox collision case stays out of scope. */

typedef struct { double x0, y0, x1, y1; } wc_box;

int fastchart_wordcloud_render_to_target(fastchart_wordcloud_obj *self, fastchart_target_t *t)
{
    fastchart_begin_render((fastchart_obj *)self, t);

    fastchart_palette pal;
    fastchart_palette_init(t, (int)self->theme, &pal);
    fastchart_palette_apply_overrides(t, (fastchart_obj *)self, &pal);

    int W, H;
    fastchart_target_get_dims(t, &W, &H);
    fastchart_target_rect(t, 0, 0, W, H, pal.bg, 1, 0);

    if (self->word_count <= 0) {
        zend_throw_error(NULL,
            "FastChart\\WordCloud::draw() requires setWords()");
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

    const char *font = fastchart_resolve_font((fastchart_obj *)self, FC_FONT_LABEL);
    if (!font) return 0;   /* no font => nothing measurable to place */

    int n = self->word_count;
    int *order = ecalloc(n, sizeof(int));
    for (int i = 0; i < n; i++) order[i] = i;
    /* Insertion sort by weight desc (n is small for a legible cloud). */
    for (int i = 1; i < n; i++) {
        int key = order[i], j = i - 1;
        while (j >= 0 && self->words[order[j]].weight < self->words[key].weight) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = key;
    }

    double wmin = self->words[0].weight, wmax = self->words[0].weight;
    for (int i = 1; i < n; i++) {
        if (self->words[i].weight < wmin) wmin = self->words[i].weight;
        if (self->words[i].weight > wmax) wmax = self->words[i].weight;
    }
    double max_font = (W < H ? W : H) / 8.0;
    if (max_font < base_size) max_font = base_size;
    if (max_font > 96.0) max_font = 96.0;
    double min_font = max_font / 4.0;
    if (min_font < 8.0) min_font = 8.0;

    int plot_x0 = 8, plot_x1 = W - 8;
    int plot_y0 = top_pad + 6, plot_y1 = H - 8;
    double cx = (plot_x0 + plot_x1) / 2.0;
    double cy = (plot_y0 + plot_y1) / 2.0;

    wc_box *placed = ecalloc(n, sizeof(*placed));
    int placed_n = 0;

    for (int oi = 0; oi < n; oi++) {
        int wi = order[oi];
        const char *text = self->words[wi].text;
        if (!text || !*text) continue;

        double fs;
        if (wmax - wmin < 1e-9) fs = (min_font + max_font) / 2.0;
        else fs = min_font + (self->words[wi].weight - wmin) / (wmax - wmin)
                  * (max_font - min_font);

        int tw = 0, th = 0;
        if (fastchart_text_measure(t, font, fs, text, &tw, &th, NULL, 0) != 0) continue;
        if (tw <= 0 || th <= 0) continue;
        double hw = tw / 2.0, hh = th / 2.0;

        /* Spiral outward until the box clears every placed box. */
        double bx = cx, by = cy;
        int ok = 0;
        for (long s = 0; s < 60000 && !ok; s++) {
            double tt = (double)s;
            double ang = tt * 2.39996322972865332;
            double rad = 1.5 * sqrt(tt);
            bx = cx + rad * cos(ang);
            by = cy + rad * sin(ang);
            wc_box cand = { bx - hw, by - hh, bx + hw, by + hh };
            if (cand.x0 < plot_x0 || cand.x1 > plot_x1 ||
                cand.y0 < plot_y0 || cand.y1 > plot_y1) {
                /* Out of bounds: keep spiralling. A word that never fits
                 * inside the plot rect is dropped below (ok stays 0). */
                continue;
            }
            ok = 1;
            for (int p = 0; p < placed_n; p++) {
                if (!(cand.x1 < placed[p].x0 - 1 || cand.x0 > placed[p].x1 + 1 ||
                      cand.y1 < placed[p].y0 - 1 || cand.y0 > placed[p].y1 + 1)) {
                    ok = 0;
                    break;
                }
            }
        }
        if (!ok) continue;   /* could not fit this word; drop it */

        placed[placed_n].x0 = bx - hw;
        placed[placed_n].y0 = by - hh;
        placed[placed_n].x1 = bx + hw;
        placed[placed_n].y1 = by + hh;
        placed_n++;

        /* Palette colour keyed on the word's stable index, not its
         * weight rank, so a word keeps its colour when weights change. */
        int color = self->words[wi].color_rgb >= 0
            ? fastchart_target_color_rgb(t, self->words[wi].color_rgb)
            : pal.series[wi % FASTCHART_PALETTE_SERIES_N];
        fastchart_text_draw(t, font, fs, color,
                            (int)bx, (int)(by + th * 0.35),
                            FASTCHART_ALIGN_CENTER, text, NULL, 0);
    }

    if (self->title && ZSTR_LEN(self->title) > 0 && title_font && title_h > 0) {
        fastchart_text_draw(t, title_font, title_size, pal.text,
                            W / 2, 12 + title_h, FASTCHART_ALIGN_CENTER,
                            ZSTR_VAL(self->title), NULL, 0);
    }

    efree(order);
    efree(placed);
    fastchart_draw_text_annotations(t, (fastchart_obj *)self, &pal);
    return 0;
}
