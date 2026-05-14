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
#include <string.h>
#include <gd.h>

#include "fastchart_text.h"
#include "fastchart_target.h"
#include "fastchart_svg.h"

/* gdImageStringFT documents two negative-fg behaviors: with no
 * gdFTEX_RESOLUTION extra setting, plain negation enables the
 * antialiasing path. Use that for visible text. */
static int aa(int color) { return -(color); }

/* Per-line vertical advance used when the input string carries
 * embedded newlines. Probes the "Mg9j" extents at the chart's DPI
 * so the advance reflects FreeType's actual ascender + descender at
 * that size, then adds a 20% leading. Falls back to size*1.4 if
 * measurement fails. */
static int line_advance(gdImagePtr im, const char *font_path, double font_size)
{
    gdFTStringExtra strex;
    memset(&strex, 0, sizeof(strex));
    int dpi = im ? (int)gdImageResolutionX(im) : 0;
    if (dpi > 0) {
        strex.flags = gdFTEX_RESOLUTION;
        strex.hdpi  = dpi;
        strex.vdpi  = dpi;
    }
    int brect[8];
    char *err = gdImageStringFTEx(NULL, brect, -1,
                                  (char *)font_path, font_size, 0.0, 0, 0,
                                  "Mg9j", &strex);
    if (err) return (int)(font_size * 1.4 + 0.5);
    int h = brect[1] - brect[7];
    if (h <= 0) return (int)(font_size * 1.4 + 0.5);
    return h * 6 / 5;  /* +20% leading */
}

/* Build a gdFTStringExtra that hands the gdImage's resolution
 * setting through to FreeType. Without this, FreeType defaults to
 * 100 DPI internally regardless of what the user set via
 * gdImageResolution(). The flag is only emitted when the canvas
 * actually has a non-zero resolution stamped — for measurement
 * paths with no canvas, we pass DPI=im_dpi explicitly so the
 * measured bounds match what the eventual draw will produce. */
static void prep_strex(gdFTStringExtra *strex, int dpi)
{
    memset(strex, 0, sizeof(*strex));
    if (dpi > 0) {
        strex->flags = gdFTEX_RESOLUTION;
        strex->hdpi  = dpi;
        strex->vdpi  = dpi;
    }
}

static int canvas_dpi(gdImagePtr im)
{
    if (!im) return 0;
    int d = (int)gdImageResolutionX(im);
    return d > 0 ? d : 0;
}

/* Map fastchart_align to FASTCHART_TARGET_ALIGN_*. The numeric values
 * happen to match today; the explicit translation keeps the layers
 * decoupled if either enum is re-ordered. */
static int align_to_target(fastchart_align a)
{
    switch (a) {
        case FASTCHART_ALIGN_CENTER: return FASTCHART_TARGET_ALIGN_CENTER;
        case FASTCHART_ALIGN_RIGHT:  return FASTCHART_TARGET_ALIGN_RIGHT;
        case FASTCHART_ALIGN_LEFT:
        default:                     return FASTCHART_TARGET_ALIGN_LEFT;
    }
}

/* Single-line draw — the core path used both for plain strings and
 * for each line of a multi-line string. Returns 0 on success or -1
 * on FreeType failure (libgd writes a message to *err which we
 * propagate to err_buf). */
static int draw_single_line(gdImagePtr im,
                            const char *font_path, double font_size,
                            int color, int x, int y,
                            fastchart_align align,
                            const char *text,
                            char *err_buf, size_t err_buf_n)
{
    gdFTStringExtra strex;
    prep_strex(&strex, canvas_dpi(im));

    int brect[8];
    char *err;

    if (align != FASTCHART_ALIGN_LEFT) {
        /* Measure first to translate the anchor point. brect indices
         * 0,1 = lower-left; 2,3 = lower-right; 4,5 = upper-right;
         * 6,7 = upper-left. text width is brect[2] - brect[6]. */
        err = gdImageStringFTEx(NULL, brect, aa(color),
                                (char *)font_path, font_size, 0.0, 0, 0,
                                (char *)text, &strex);
        if (err != NULL) {
            if (err_buf && err_buf_n) snprintf(err_buf, err_buf_n, "%s", err);
            return -1;
        }
        int w = brect[2] - brect[6];
        if (align == FASTCHART_ALIGN_CENTER) x -= w / 2;
        else                                  x -= w;
    }

    err = gdImageStringFTEx(im, brect, aa(color),
                            (char *)font_path, font_size, 0.0, x, y,
                            (char *)text, &strex);
    if (err != NULL) {
        if (err_buf && err_buf_n) snprintf(err_buf, err_buf_n, "%s", err);
        return -1;
    }
    return 0;
}

int fastchart_text_draw(fastchart_target_t *t,
                        const char *font_path, double font_size,
                        int color, int x, int y,
                        fastchart_align align,
                        const char *text,
                        char *err_buf, size_t err_buf_n)
{
    if (!font_path || !*font_path) {
        if (err_buf && err_buf_n) snprintf(err_buf, err_buf_n, "no font path set");
        return -1;
    }
    if (!text || !*text) return 0;  /* nothing to draw */

    if (t->kind == FASTCHART_TARGET_GD) {
        gdImagePtr im = t->u.gd.im;
        int gd_color = fastchart_target_color_to_gd(t, color);
        if (gd_color < 0) return -1;

        /* Common case: no newline. Single-line draw at (x, y). */
        const char *nl = strchr(text, '\n');
        if (!nl) {
            return draw_single_line(im, font_path, font_size, gd_color, x, y,
                                    align, text, err_buf, err_buf_n);
        }

        /* Multi-line: each line is drawn at the same x with its
         * baseline stepping down by `adv`. The caller-supplied (x, y)
         * anchors the FIRST line, matching the single-line semantics
         * so existing layout math doesn't shift. */
        int adv = line_advance(im, font_path, font_size);

        /* Walk in-place, copying each line into a stack buffer that's
         * NUL-terminated. Lines longer than the buffer are truncated;
         * 1024 chars is far above any realistic title / label length. */
        char buf[1024];
        int line_y = y;
        const char *p = text;
        while (1) {
            const char *end = strchr(p, '\n');
            size_t len = end ? (size_t)(end - p) : strlen(p);
            if (len >= sizeof(buf)) len = sizeof(buf) - 1;
            memcpy(buf, p, len);
            buf[len] = '\0';

            if (len > 0) {
                if (draw_single_line(im, font_path, font_size, gd_color, x, line_y,
                                     align, buf, err_buf, err_buf_n) != 0) {
                    return -1;
                }
            }
            if (!end) break;
            p = end + 1;
            line_y += adv;
        }
        return 0;
    }

    /* SVG path. text-anchor handles horizontal alignment so we don't
     * pre-measure; pass the alignment hint through and emit one
     * <text> per line. size_pt -> CSS px at 96 DPI baseline:
     * px = pt * 4/3. 20% leading matches the GD path's line advance.
     * Branch on the target's text mode: PATHS flattens to <path>
     * glyphs via FreeType; NATIVE keeps <text>. */
    char family[64];
    fastchart_target_resolve_font_family(t, font_path, family, sizeof(family));
    double size_px = font_size * (4.0 / 3.0);
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    int svg_align = align_to_target(align);
    double line_step = size_px * 1.2;

    char buf[1024];
    double line_y = (double)y;
    const char *p = text;
    while (1) {
        const char *end = strchr(p, '\n');
        size_t len = end ? (size_t)(end - p) : strlen(p);
        if (len >= sizeof(buf)) len = sizeof(buf) - 1;
        memcpy(buf, p, len);
        buf[len] = '\0';

        if (len > 0) {
            if (t->u.svg.text_mode == FASTCHART_SVG_TEXT_PATHS) {
                fc_svg_emit_text_as_path(t->u.svg.buf, (double)x, line_y,
                                          font_path, size_px, rgba,
                                          0.0, svg_align, buf, len);
            } else {
                fc_svg_emit_text(t->u.svg.buf, (double)x, line_y,
                                 family, size_px, rgba,
                                 0.0, svg_align, buf, len);
            }
        }
        if (!end) break;
        p = end + 1;
        line_y += line_step;
    }
    return 0;
}

int fastchart_text_draw_rotated(fastchart_target_t *t,
                                const char *font_path, double font_size,
                                int color, int x, int y,
                                fastchart_align align, double angle_deg,
                                const char *text,
                                char *err_buf, size_t err_buf_n)
{
    if (!font_path || !*font_path) {
        if (err_buf && err_buf_n) snprintf(err_buf, err_buf_n, "no font path set");
        return -1;
    }
    if (!text || !*text) return 0;

    if (t->kind == FASTCHART_TARGET_GD) {
        gdImagePtr im = t->u.gd.im;
        int gd_color = fastchart_target_color_to_gd(t, color);
        if (gd_color < 0) return -1;

        /* gdImageStringFT angle is in radians, counter-clockwise. */
        double rad = angle_deg * (3.14159265358979323846 / 180.0);

        gdFTStringExtra strex;
        prep_strex(&strex, canvas_dpi(im));

        int brect[8];
        char *err;

        /* Measure horizontally first so we can translate the anchor
         * for non-LEFT alignment. The shift must be applied along the
         * rotated baseline direction — libgd's (x, y) is where the
         * FIRST glyph's lower-left lands on the canvas, then the text
         * walks at `angle` from there. For ALIGN_RIGHT we want the
         * LAST glyph's baseline end to land at (x, y); that means
         * starting at
         *   (x - w*cos θ, y + w*sin θ)
         * (Y inverts because libgd CCW rotates against image-Y-down).
         * The previous version shifted by (-w, 0) regardless of
         * angle, which placed rotated tick labels visibly off their
         * tick. */
        if (align != FASTCHART_ALIGN_LEFT) {
            err = gdImageStringFTEx(NULL, brect, -(gd_color),
                                    (char *)font_path, font_size, 0.0, 0, 0,
                                    (char *)text, &strex);
            if (err != NULL) {
                if (err_buf && err_buf_n) snprintf(err_buf, err_buf_n, "%s", err);
                return -1;
            }
            int w = brect[2] - brect[6];
            double shift = (align == FASTCHART_ALIGN_CENTER) ? (double)w / 2.0
                                                             : (double)w;
            x -= (int)(shift * cos(rad) + 0.5);
            y += (int)(shift * sin(rad) + 0.5);
        }

        err = gdImageStringFTEx(im, brect, -(gd_color),
                                (char *)font_path, font_size, rad, x, y,
                                (char *)text, &strex);
        if (err != NULL) {
            if (err_buf && err_buf_n) snprintf(err_buf, err_buf_n, "%s", err);
            return -1;
        }
        return 0;
    }

    /* SVG path. fc_svg_emit_text handles rotation via transform=
     * "rotate(angle, x, y)"; text-anchor handles alignment. No
     * pre-measurement needed. Branch on text mode same as the
     * single-line path. */
    char family[64];
    fastchart_target_resolve_font_family(t, font_path, family, sizeof(family));
    double size_px = font_size * (4.0 / 3.0);
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    int svg_align = align_to_target(align);
    if (t->u.svg.text_mode == FASTCHART_SVG_TEXT_PATHS) {
        fc_svg_emit_text_as_path(t->u.svg.buf, (double)x, (double)y,
                                  font_path, size_px, rgba,
                                  angle_deg, svg_align, text, strlen(text));
    } else {
        fc_svg_emit_text(t->u.svg.buf, (double)x, (double)y,
                         family, size_px, rgba,
                         angle_deg, svg_align, text, strlen(text));
    }
    return 0;
}

int fastchart_text_measure(fastchart_target_t *t,
                           const char *font_path, double font_size,
                           const char *text,
                           int *out_w, int *out_h,
                           char *err_buf, size_t err_buf_n)
{
    if (!font_path || !*font_path) {
        if (err_buf && err_buf_n) {
            snprintf(err_buf, err_buf_n, "no font path set");
        }
        return -1;
    }
    if (!text || !*text) {
        if (out_w) *out_w = 0;
        if (out_h) *out_h = 0;
        return 0;
    }

    /* FreeType measurement is canvas-independent — pass the target's
     * effective DPI directly instead of probing a gdImage. Same
     * measurement on GD and SVG backends, so layout stays stable. */
    int dpi = t ? fastchart_target_get_dpi(t) : 0;
    /* line_advance still wants a gdImage for the canvas-DPI probe,
     * but it falls back to size*1.4 on NULL — we feed the GD-side
     * im when available so the AA-resolution stamp is consulted; for
     * SVG, NULL forces the heuristic path (acceptable: SVG layout
     * stability matters less than the GD path's exact ascender).
     *
     * FUTURE: refactor line_advance to take a dpi int directly. */
    gdImagePtr im = (t && t->kind == FASTCHART_TARGET_GD) ? t->u.gd.im : NULL;

    /* Pass the canvas DPI to FreeType so the measured bounds match
     * what the draw will produce. Layout that measures at 96 DPI but
     * draws at 200 DPI will be off by ~2x. */
    gdFTStringExtra strex;
    prep_strex(&strex, dpi);

    /* Multi-line: width = max line width, height = first ascender +
     * (n_lines - 1) * advance. Mirror the draw path's per-line
     * stepping so layout reservations match what gets drawn. */
    if (strchr(text, '\n')) {
        int adv = line_advance(im, font_path, font_size);
        int max_w = 0, n_lines = 0;
        int first_h = 0;
        char buf[1024];
        const char *p = text;
        while (1) {
            const char *end = strchr(p, '\n');
            size_t len = end ? (size_t)(end - p) : strlen(p);
            if (len >= sizeof(buf)) len = sizeof(buf) - 1;
            memcpy(buf, p, len);
            buf[len] = '\0';
            n_lines++;
            if (len > 0) {
                int brect[8];
                char *err = gdImageStringFTEx(NULL, brect, -1,
                                              (char *)font_path, font_size, 0.0, 0, 0,
                                              buf, &strex);
                if (err == NULL) {
                    int w = brect[2] - brect[6];
                    if (w > max_w) max_w = w;
                    if (first_h == 0) first_h = brect[1] - brect[7];
                }
            }
            if (!end) break;
            p = end + 1;
        }
        if (out_w) *out_w = max_w;
        if (out_h) *out_h = first_h + (n_lines - 1) * adv;
        return 0;
    }

    int brect[8];
    char *err = gdImageStringFTEx(NULL, brect, -1,
                                  (char *)font_path, font_size, 0.0, 0, 0,
                                  (char *)text, &strex);
    if (err != NULL) {
        if (err_buf && err_buf_n) {
            snprintf(err_buf, err_buf_n, "%s", err);
        }
        return -1;
    }
    if (out_w) *out_w = brect[2] - brect[6];
    if (out_h) *out_h = brect[1] - brect[7];
    return 0;
}
