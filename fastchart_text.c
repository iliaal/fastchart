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
#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "fastchart_text.h"
#include "fastchart_target.h"
#include "fastchart_svg.h"

/* Measure a UTF-8 byte range at the given point size + DPI using
 * FreeType directly. text_len is the byte count (NOT codepoint count);
 * the function does NOT require NUL termination, so callers can pass
 * arbitrary slices of a larger buffer. out_w / out_h receive the
 * pixel-space width and the ascender-to-descender height. Returns 0
 * on success, -1 on FT failure. Width sums each glyph's advance.x
 * (in 26.6 fixed-point pixels). Height is the face's bbox in pixel
 * units at the configured size. */
static int fc_ft_measure(const char *font_path, double size_pt, int dpi,
                         const char *text, size_t text_len,
                         int *out_w, int *out_h)
{
    if (!font_path || !text) return -1;
    if (dpi <= 0) dpi = 96;

    /* Shared FT_Face from the process-wide cache; the cache owns
     * the lifetime, do not call FT_Done_Face on the returned ptr. */
    FT_Face face = fastchart_ft_face(font_path);
    if (!face) return -1;
    /* Size in 1/64 of a point at the given DPI. Always set: the
     * cached face's size carries over from whatever the last caller
     * configured. */
    if (FT_Set_Char_Size(face, (FT_F26Dot6)(size_pt * 64.0), 0, dpi, dpi)) {
        return -1;
    }

    long total_w_64 = 0;
    const unsigned char *p = (const unsigned char *)text;
    const unsigned char *end = p + text_len;
    while (p < end) {
        unsigned cp;
        if (*p < 0x80) { cp = *p++; }
        else if ((*p & 0xE0) == 0xC0 && (end - p) >= 2) {
            cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
            p += 2;
        } else if ((*p & 0xF0) == 0xE0 && (end - p) >= 3) {
            cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            p += 3;
        } else if ((*p & 0xF8) == 0xF0 && (end - p) >= 4) {
            /* 4-byte sequence covers U+10000 and above — emoji, CJK
             * extensions, mathematical alphanumeric symbols. Without
             * this branch a non-BMP codepoint contributed 0 to the
             * measured width and chart layout reservations came up
             * systematically narrow for any label containing emoji. */
            cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12)
               | ((p[2] & 0x3F) << 6)  |  (p[3] & 0x3F);
            p += 4;
        } else { p++; continue; }

        FT_UInt gi = FT_Get_Char_Index(face, cp);
        if (FT_Load_Glyph(face, gi, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING))
            continue;
        total_w_64 += face->glyph->advance.x;
    }

    int w = (int)((total_w_64 + 32) / 64);  /* round to nearest pixel */
    /* Height: ascender + |descender| (both in font units, scale to px).
     * units_per_EM is the design-space unit count; size_px is the
     * pixel size at the current DPI. */
    double size_px = size_pt * (double)dpi / 72.0;
    int h;
    if (face->units_per_EM > 0) {
        h = (int)((face->ascender - face->descender) * size_px
                  / face->units_per_EM + 0.5);
    } else {
        h = (int)(size_px + 0.5);
    }

    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
    return 0;
}

/* Per-line vertical advance for multi-line strings. ascender +
 * descender + 20% leading. Falls back to size*1.4 on FT failure. */
static int line_advance(int dpi, const char *font_path, double font_size)
{
    int w, h;
    if (fc_ft_measure(font_path, font_size, dpi, "Mg9j", 4, &w, &h) != 0) {
        return (int)(font_size * 1.4 + 0.5);
    }
    if (h <= 0) return (int)(font_size * 1.4 + 0.5);
    return h * 6 / 5;  /* +20% leading */
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

    /* Emit each line directly from the source pointer + byte length.
     * fc_svg_emit_text and fc_svg_emit_text_as_path both accept a
     * (text, len) pair, so no intermediate NUL-terminated copy is
     * needed — that removes the old byte-truncating char buf[1024]
     * which could split a multi-byte UTF-8 sequence mid-line and
     * silently drop the tail of long single-line labels. */
    double line_y = (double)y;
    const char *p = text;
    while (1) {
        const char *end = strchr(p, '\n');
        size_t len = end ? (size_t)(end - p) : strlen(p);

        if (len > 0) {
            if (t->u.svg.text_mode == FASTCHART_SVG_TEXT_PATHS) {
                fc_svg_emit_text_as_path(t->u.svg.buf, (double)x, line_y,
                                          font_path, size_px, rgba,
                                          0.0, svg_align, p, len);
            } else {
                fc_svg_emit_text(t->u.svg.buf, (double)x, line_y,
                                 family, size_px, rgba,
                                 0.0, svg_align, p, len);
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

    /* SVG path. fc_svg_emit_text handles rotation via transform=
     * "rotate(angle, x, y)"; text-anchor handles alignment.
     *
     * Multi-line: split on \n and emit one rotated <text> per line.
     * Each line's anchor is offset PERPENDICULAR to the rotation
     * direction so the lines stack along the rotated y-axis. For
     * rotation by angle θ (CCW), a vertical line-step of S becomes:
     *   delta_x = -S * sin(θ)
     *   delta_y =  S * cos(θ)
     * The unrotated path emitted one element with raw \n bytes in
     * the body; SVG <text> collapses \n to whitespace at render
     * time, so the multi-line label rendered as a single line —
     * while the companion measure() function reports the multi-line
     * height, so layout and rendering disagreed. */
    char family[64];
    fastchart_target_resolve_font_family(t, font_path, family, sizeof(family));
    double size_px = font_size * (4.0 / 3.0);
    uint32_t rgba = fastchart_target_color_to_rgba(t, color);
    int svg_align = align_to_target(align);
    double line_step = size_px * 1.2;
    double rad = angle_deg * M_PI / 180.0;
    double sin_t = sin(rad);
    double cos_t = cos(rad);

    int line_no = 0;
    const char *p = text;
    while (1) {
        const char *end = strchr(p, '\n');
        size_t len = end ? (size_t)(end - p) : strlen(p);

        if (len > 0) {
            double offset = line_no * line_step;
            double line_x = (double)x - offset * sin_t;
            double line_y = (double)y + offset * cos_t;
            if (t->u.svg.text_mode == FASTCHART_SVG_TEXT_PATHS) {
                fc_svg_emit_text_as_path(t->u.svg.buf, line_x, line_y,
                                          font_path, size_px, rgba,
                                          angle_deg, svg_align, p, len);
            } else {
                fc_svg_emit_text(t->u.svg.buf, line_x, line_y,
                                 family, size_px, rgba,
                                 angle_deg, svg_align, p, len);
            }
        }
        if (!end) break;
        p = end + 1;
        line_no++;
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

    /* FreeType measurement at the target's DPI. */
    int dpi = t ? fastchart_target_get_dpi(t) : 96;

    /* Multi-line: width = max line width, height = first ascender +
     * (n_lines - 1) * advance. Mirror the draw path's per-line
     * stepping so layout reservations match what gets drawn. */
    if (strchr(text, '\n')) {
        int adv = line_advance(dpi, font_path, font_size);
        int max_w = 0, n_lines = 0;
        int first_h = 0;
        const char *p = text;
        while (1) {
            const char *end = strchr(p, '\n');
            size_t len = end ? (size_t)(end - p) : strlen(p);
            n_lines++;
            if (len > 0) {
                int w = 0, h = 0;
                if (fc_ft_measure(font_path, font_size, dpi, p, len, &w, &h) == 0) {
                    if (w > max_w) max_w = w;
                    if (first_h == 0) first_h = h;
                }
            }
            if (!end) break;
            p = end + 1;
        }
        if (out_w) *out_w = max_w;
        if (out_h) *out_h = first_h + (n_lines - 1) * adv;
        return 0;
    }

    int w = 0, h = 0;
    if (fc_ft_measure(font_path, font_size, dpi, text, strlen(text), &w, &h) != 0) {
        if (err_buf && err_buf_n) {
            snprintf(err_buf, err_buf_n, "FreeType measure failed");
        }
        return -1;
    }
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
    return 0;
}
