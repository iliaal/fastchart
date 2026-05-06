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

int fastchart_text_draw(gdImagePtr im,
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

    /* Common case: no newline. Single-line draw at (x, y). */
    const char *nl = strchr(text, '\n');
    if (!nl) {
        return draw_single_line(im, font_path, font_size, color, x, y,
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
            if (draw_single_line(im, font_path, font_size, color, x, line_y,
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

int fastchart_text_draw_rotated(gdImagePtr im,
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

    /* gdImageStringFT angle is in radians, counter-clockwise. */
    double rad = angle_deg * (3.14159265358979323846 / 180.0);

    gdFTStringExtra strex;
    prep_strex(&strex, canvas_dpi(im));

    int brect[8];
    char *err;

    /* Measure horizontally first so we can translate the anchor for
     * non-LEFT alignment. The shift must be applied along the rotated
     * baseline direction — libgd's (x, y) is where the FIRST glyph's
     * lower-left lands on the canvas, then the text walks at `angle`
     * from there. For ALIGN_RIGHT we want the LAST glyph's baseline
     * end to land at (x, y); that means starting at
     *   (x - w*cos θ, y + w*sin θ)
     * (Y inverts because libgd CCW rotates against image-Y-down).
     * The previous version shifted by (-w, 0) regardless of angle,
     * which placed rotated tick labels visibly off their tick. */
    if (align != FASTCHART_ALIGN_LEFT) {
        err = gdImageStringFTEx(NULL, brect, -(color),
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

    err = gdImageStringFTEx(im, brect, -(color),
                            (char *)font_path, font_size, rad, x, y,
                            (char *)text, &strex);
    if (err != NULL) {
        if (err_buf && err_buf_n) snprintf(err_buf, err_buf_n, "%s", err);
        return -1;
    }
    return 0;
}

int fastchart_text_measure(gdImagePtr im,
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

    /* Pass the canvas DPI to FreeType so the measured bounds match
     * what the draw will produce. Layout that measures at 96 DPI but
     * draws at 200 DPI will be off by ~2x. */
    gdFTStringExtra strex;
    prep_strex(&strex, canvas_dpi(im));

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
