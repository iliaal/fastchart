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

#include <string.h>
#include <gd.h>

#include "fastchart_text.h"

/* gdImageStringFT documents two negative-fg behaviors: with no
 * gdFTEX_RESOLUTION extra setting, plain negation enables the
 * antialiasing path. Use that for visible text. */
static int aa(int color) { return -(color); }

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

int fastchart_text_draw(gdImagePtr im,
                        const char *font_path, double font_size,
                        int color, int x, int y,
                        fastchart_align align,
                        const char *text,
                        char *err_buf, size_t err_buf_n)
{
    if (!font_path || !*font_path) {
        if (err_buf && err_buf_n) {
            snprintf(err_buf, err_buf_n, "no font path set");
        }
        return -1;
    }
    if (!text || !*text) {
        return 0;  /* nothing to draw */
    }

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
            if (err_buf && err_buf_n) {
                snprintf(err_buf, err_buf_n, "%s", err);
            }
            return -1;
        }
        int w = brect[2] - brect[6];
        if (align == FASTCHART_ALIGN_CENTER) {
            x -= w / 2;
        } else { /* RIGHT */
            x -= w;
        }
    }

    err = gdImageStringFTEx(im, brect, aa(color),
                            (char *)font_path, font_size, 0.0, x, y,
                            (char *)text, &strex);
    if (err != NULL) {
        if (err_buf && err_buf_n) {
            snprintf(err_buf, err_buf_n, "%s", err);
        }
        return -1;
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
     * non-LEFT alignment in the unrotated frame. */
    if (align != FASTCHART_ALIGN_LEFT) {
        err = gdImageStringFTEx(NULL, brect, -(color),
                                (char *)font_path, font_size, 0.0, 0, 0,
                                (char *)text, &strex);
        if (err != NULL) {
            if (err_buf && err_buf_n) snprintf(err_buf, err_buf_n, "%s", err);
            return -1;
        }
        int w = brect[2] - brect[6];
        if (align == FASTCHART_ALIGN_CENTER) x -= w / 2;
        else if (align == FASTCHART_ALIGN_RIGHT) x -= w;
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
