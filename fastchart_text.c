/*
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2026 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,     |
  | that is bundled with this package in the file LICENSE, and is       |
  | available through the world-wide-web at the following url:          |
  | http://www.php.net/license/3_01.txt                                 |
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

    int brect[8];
    char *err;

    if (align != FASTCHART_ALIGN_LEFT) {
        /* Measure first to translate the anchor point. brect indices
         * 0,1 = lower-left; 2,3 = lower-right; 4,5 = upper-right;
         * 6,7 = upper-left. text width is brect[2] - brect[6]. */
        err = gdImageStringFT(NULL, brect, aa(color),
                              (char *)font_path, font_size, 0.0, 0, 0,
                              (char *)text);
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

    err = gdImageStringFT(im, brect, aa(color),
                          (char *)font_path, font_size, 0.0, x, y,
                          (char *)text);
    if (err != NULL) {
        if (err_buf && err_buf_n) {
            snprintf(err_buf, err_buf_n, "%s", err);
        }
        return -1;
    }
    return 0;
}

int fastchart_text_measure(const char *font_path, double font_size,
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

    int brect[8];
    char *err = gdImageStringFT(NULL, brect, -1,
                                (char *)font_path, font_size, 0.0, 0, 0,
                                (char *)text);
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
