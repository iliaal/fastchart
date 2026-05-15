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

  Code 128 (ISO/IEC 15417) implementation. Supports subsets A, B, C
  with auto-switching to minimise encoded length:
    - Subset A covers ASCII control chars (0..31) plus uppercase /
      digits / common symbols (32..95).
    - Subset B covers printable ASCII 32..127.
    - Subset C encodes pairs of digits ("00".."99") in a single code,
      doubling density on numeric runs.

  The encoder picks the start subset by leading content (digit run
  → C, control char → A, otherwise B) and switches mid-stream when
  the input shape changes (digit run of 4+ → C; lowercase run from
  A → B; isolated char → SHIFT). Mod-103 weighted checksum is
  appended automatically.

  Bar pattern table sourced from JsBarcode's CODE128 constants
  (lindell/JsBarcode), cross-checked against ISO/IEC 15417 by
  width-sum invariants (11 modules per data symbol, 13 for stop)
  and the canonical START_C / STOP encodings.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "fastchart_target.h"
#include "fastchart_text.h"

/* Code 128 special code values. Same numeric values across subsets;
 * their meaning depends on the active subset as listed below. */
#define C128_FNC1     102  /* Same in A, B, C */
#define C128_CODE_A   101  /* In B/C: switch to A */
#define C128_CODE_B   100  /* In A/C: switch to B */
#define C128_CODE_C    99  /* In A/B: switch to C */
#define C128_SHIFT     98  /* In A/B: next char is in the OTHER subset */
#define C128_START_A  103
#define C128_START_B  104
#define C128_START_C  105
#define C128_STOP     106

/* Bar/space width patterns for Code 128. Each entry sums to 11
 * modules (3 bars + 3 spaces, alternating starting with a bar)
 * except the stop pattern, which is 13 modules across 7 widths.
 * Generated from JsBarcode's BARS array via PHP one-liner; verified
 * against the canonical START_C={2,1,1,2,3,2} and STOP={2,3,3,1,1,1,2}. */
static const uint8_t code128_patterns[106][6] = {
    {2,1,2,2,2,2},  /*   0 */
    {2,2,2,1,2,2},  /*   1 */
    {2,2,2,2,2,1},  /*   2 */
    {1,2,1,2,2,3},  /*   3 */
    {1,2,1,3,2,2},  /*   4 */
    {1,3,1,2,2,2},  /*   5 */
    {1,2,2,2,1,3},  /*   6 */
    {1,2,2,3,1,2},  /*   7 */
    {1,3,2,2,1,2},  /*   8 */
    {2,2,1,2,1,3},  /*   9 */
    {2,2,1,3,1,2},  /*  10 */
    {2,3,1,2,1,2},  /*  11 */
    {1,1,2,2,3,2},  /*  12 */
    {1,2,2,1,3,2},  /*  13 */
    {1,2,2,2,3,1},  /*  14 */
    {1,1,3,2,2,2},  /*  15 */
    {1,2,3,1,2,2},  /*  16 */
    {1,2,3,2,2,1},  /*  17 */
    {2,2,3,2,1,1},  /*  18 */
    {2,2,1,1,3,2},  /*  19 */
    {2,2,1,2,3,1},  /*  20 */
    {2,1,3,2,1,2},  /*  21 */
    {2,2,3,1,1,2},  /*  22 */
    {3,1,2,1,3,1},  /*  23 */
    {3,1,1,2,2,2},  /*  24 */
    {3,2,1,1,2,2},  /*  25 */
    {3,2,1,2,2,1},  /*  26 */
    {3,1,2,2,1,2},  /*  27 */
    {3,2,2,1,1,2},  /*  28 */
    {3,2,2,2,1,1},  /*  29 */
    {2,1,2,1,2,3},  /*  30 */
    {2,1,2,3,2,1},  /*  31 */
    {2,3,2,1,2,1},  /*  32 */
    {1,1,1,3,2,3},  /*  33 */
    {1,3,1,1,2,3},  /*  34 */
    {1,3,1,3,2,1},  /*  35 */
    {1,1,2,3,1,3},  /*  36 */
    {1,3,2,1,1,3},  /*  37 */
    {1,3,2,3,1,1},  /*  38 */
    {2,1,1,3,1,3},  /*  39 */
    {2,3,1,1,1,3},  /*  40 */
    {2,3,1,3,1,1},  /*  41 */
    {1,1,2,1,3,3},  /*  42 */
    {1,1,2,3,3,1},  /*  43 */
    {1,3,2,1,3,1},  /*  44 */
    {1,1,3,1,2,3},  /*  45 */
    {1,1,3,3,2,1},  /*  46 */
    {1,3,3,1,2,1},  /*  47 */
    {3,1,3,1,2,1},  /*  48 */
    {2,1,1,3,3,1},  /*  49 */
    {2,3,1,1,3,1},  /*  50 */
    {2,1,3,1,1,3},  /*  51 */
    {2,1,3,3,1,1},  /*  52 */
    {2,1,3,1,3,1},  /*  53 */
    {3,1,1,1,2,3},  /*  54 */
    {3,1,1,3,2,1},  /*  55 */
    {3,3,1,1,2,1},  /*  56 */
    {3,1,2,1,1,3},  /*  57 */
    {3,1,2,3,1,1},  /*  58 */
    {3,3,2,1,1,1},  /*  59 */
    {3,1,4,1,1,1},  /*  60 */
    {2,2,1,4,1,1},  /*  61 */
    {4,3,1,1,1,1},  /*  62 */
    {1,1,1,2,2,4},  /*  63 */
    {1,1,1,4,2,2},  /*  64 */
    {1,2,1,1,2,4},  /*  65 */
    {1,2,1,4,2,1},  /*  66 */
    {1,4,1,1,2,2},  /*  67 */
    {1,4,1,2,2,1},  /*  68 */
    {1,1,2,2,1,4},  /*  69 */
    {1,1,2,4,1,2},  /*  70 */
    {1,2,2,1,1,4},  /*  71 */
    {1,2,2,4,1,1},  /*  72 */
    {1,4,2,1,1,2},  /*  73 */
    {1,4,2,2,1,1},  /*  74 */
    {2,4,1,2,1,1},  /*  75 */
    {2,2,1,1,1,4},  /*  76 */
    {4,1,3,1,1,1},  /*  77 */
    {2,4,1,1,1,2},  /*  78 */
    {1,3,4,1,1,1},  /*  79 */
    {1,1,1,2,4,2},  /*  80 */
    {1,2,1,1,4,2},  /*  81 */
    {1,2,1,2,4,1},  /*  82 */
    {1,1,4,2,1,2},  /*  83 */
    {1,2,4,1,1,2},  /*  84 */
    {1,2,4,2,1,1},  /*  85 */
    {4,1,1,2,1,2},  /*  86 */
    {4,2,1,1,1,2},  /*  87 */
    {4,2,1,2,1,1},  /*  88 */
    {2,1,2,1,4,1},  /*  89 */
    {2,1,4,1,2,1},  /*  90 */
    {4,1,2,1,2,1},  /*  91 */
    {1,1,1,1,4,3},  /*  92 */
    {1,1,1,3,4,1},  /*  93 */
    {1,3,1,1,4,1},  /*  94 */
    {1,1,4,1,1,3},  /*  95 */
    {1,1,4,3,1,1},  /*  96 */
    {4,1,1,1,1,3},  /*  97 */
    {4,1,1,3,1,1},  /*  98 */
    {1,1,3,1,4,1},  /*  99 */
    {1,1,4,1,3,1},  /* 100 */
    {3,1,1,1,4,1},  /* 101 */
    {4,1,1,1,3,1},  /* 102 */
    {2,1,1,4,1,2},  /* 103 = Start A */
    {2,1,1,2,1,4},  /* 104 = Start B */
    {2,1,1,2,3,2},  /* 105 = Start C */
};
static const uint8_t code128_stop_pattern[7] = {2,3,3,1,1,1,2};

/* Hard cap on input length. Code 128 itself has no theoretical
 * limit, but practical scanners struggle past ~80 chars and the
 * encoder buffer below sizes accordingly. */
#define C128_MAX_INPUT 80
#define C128_MAX_CODES 256   /* 80 chars + start + checksum + stop with worst-case shifts */

static int count_consecutive_digits(const char *data, size_t len, size_t i)
{
    int n = 0;
    while (i + (size_t)n < len && data[i + n] >= '0' && data[i + n] <= '9') n++;
    return n;
}

/* Encode `data` into the codes buffer, returning the number of codes
 * emitted (excluding checksum and stop, which the renderer appends).
 * Returns -1 on encode failure with an English message in `err`. */
static int code128_encode(const char *data, size_t len,
                          uint8_t *codes, size_t cap,
                          char *err, size_t err_sz)
{
    if (len == 0) {
        snprintf(err, err_sz, "Code 128: input is empty");
        return -1;
    }
    if (len > C128_MAX_INPUT) {
        snprintf(err, err_sz,
            "Code 128: input is %zu chars; max supported is %d",
            len, C128_MAX_INPUT);
        return -1;
    }
    /* Reject non-ASCII bytes early. The standard Code 128 covers
     * ASCII 0..127; the FNC4 mechanism for extended ASCII (128..255)
     * is not supported in v0. */
    for (size_t k = 0; k < len; k++) {
        if ((unsigned char)data[k] > 127) {
            snprintf(err, err_sz,
                "Code 128: byte 0x%02X at position %zu is outside ASCII; "
                "use a UTF-8 → ASCII transliteration upstream or switch to QR.",
                (unsigned char)data[k], k);
            return -1;
        }
    }

    size_t out = 0;
    size_t i = 0;
    int subset = 0;  /* 0 = unset; later 'A', 'B', or 'C' */

#define EMIT(v) do { \
        if (out >= cap) { \
            snprintf(err, err_sz, "Code 128: encoder ran out of buffer space"); \
            return -1; \
        } \
        codes[out++] = (uint8_t)(v); \
    } while (0)

    /* Pick the start subset by leading shape.
     *
     *   - Pure-numeric input of even length: start in C, encode pairs.
     *   - Leading run of 4+ digits: start in C; the encoder switches
     *     out when the run ends.
     *   - First char is an ASCII control char (<32): start in A.
     *   - Otherwise: start in B (covers printable ASCII).
     *
     * The "≥4 digits" threshold matches the standard Code 128 minimum
     * worth of switching to C: 2 codes saved (one for the shift in,
     * one for the encoded pair vs. two B codes) is the break-even at
     * 4 digits; below that, B is no worse and avoids subset thrash. */
    int leading_digits = count_consecutive_digits(data, len, 0);
    int leading_is_all_data = (leading_digits == (int)len);
    if ((leading_is_all_data && leading_digits >= 2 && (leading_digits % 2) == 0) ||
        leading_digits >= 4) {
        EMIT(C128_START_C); subset = 'C';
    } else if ((unsigned char)data[0] < 32) {
        EMIT(C128_START_A); subset = 'A';
    } else {
        EMIT(C128_START_B); subset = 'B';
    }

    while (i < len) {
        unsigned char c = (unsigned char)data[i];
        int digits_here = count_consecutive_digits(data, len, i);

        if (subset == 'C') {
            /* C: encode digit pairs. If we have <2 digits, switch out. */
            if (digits_here >= 2) {
                int v = (data[i] - '0') * 10 + (data[i + 1] - '0');
                EMIT(v);
                i += 2;
                /* Consider switching out of C if the remaining digits
                 * after this pair are insufficient for another pair
                 * AND there's still input left. */
                int remaining_digits = count_consecutive_digits(data, len, i);
                if (remaining_digits < 2 && i < len) {
                    if ((unsigned char)data[i] < 32) {
                        EMIT(C128_CODE_A); subset = 'A';
                    } else {
                        EMIT(C128_CODE_B); subset = 'B';
                    }
                }
            } else {
                /* Single trailing digit (odd-length tail) or non-digit. */
                if (c < 32) {
                    EMIT(C128_CODE_A); subset = 'A';
                } else {
                    EMIT(C128_CODE_B); subset = 'B';
                }
                /* Don't advance i; re-handle in new subset. */
            }
        }
        else if (subset == 'B') {
            /* Switch to C if we have a long-enough digit run.
             *
             * Order matters: `odd_tail` must be checked BEFORE the
             * generic `≥6 || (tail_run && ≥4)` branch. For an odd
             * digit tail like 7, the generic branch sees ≥6 and
             * switches to C, which then handles 3 pairs + a single
             * trailing digit (CODE_B + 1 in B = 2 extra codes for
             * the tail). The `odd_tail` path emits one digit in B
             * first, so the remaining 6-digit even tail switches to
             * C cleanly (CODE_C + 3 pairs). One fewer code (= 11
             * fewer modules); matters because auto module_px scales
             * with total bar count. */
            int tail_run = (digits_here >= 2 && (size_t)(i + digits_here) == len &&
                            (digits_here % 2) == 0);
            int odd_tail = (digits_here >= 5 && (size_t)(i + digits_here) == len &&
                            (digits_here % 2) == 1);
            if (odd_tail) {
                /* Emit one digit in B, let the next iteration's
                 * tail_run branch switch to C for the even remainder. */
                EMIT(c - 32);
                i++;
                continue;
            }
            if (digits_here >= 6 || (tail_run && digits_here >= 4)) {
                EMIT(C128_CODE_C); subset = 'C';
                continue;
            }

            if (c >= 32 && c <= 127) {
                EMIT(c - 32);
                i++;
            } else {  /* c < 32: control char in B context */
                /* If we have ≥2 control chars, switch to A; else SHIFT. */
                int ctrls_here = 0;
                while (i + (size_t)ctrls_here < len &&
                       (unsigned char)data[i + ctrls_here] < 32) {
                    ctrls_here++;
                }
                if (ctrls_here >= 2) {
                    EMIT(C128_CODE_A); subset = 'A';
                    continue;
                }
                EMIT(C128_SHIFT);
                EMIT(c + 64);  /* A-encoding of control char */
                i++;
            }
        }
        else {  /* subset == 'A' */
            /* Same odd-tail-before-≥6 ordering as the B branch above;
             * see comment there. */
            int tail_run = (digits_here >= 2 && (size_t)(i + digits_here) == len &&
                            (digits_here % 2) == 0);
            int odd_tail = (digits_here >= 5 && (size_t)(i + digits_here) == len &&
                            (digits_here % 2) == 1);
            if (odd_tail) {
                /* Emit one digit in A, then let the next iteration's
                 * tail_run branch switch to C. Subset A maps ASCII
                 * 32..95 to values 0..63 (digits '0'..'9' → 16..25),
                 * so the encoding is identical to subset B for the
                 * digit characters. */
                EMIT(c - 32);
                i++;
                continue;
            }
            if (digits_here >= 6 || (tail_run && digits_here >= 4)) {
                EMIT(C128_CODE_C); subset = 'C';
                continue;
            }

            if (c < 32) {
                EMIT(c + 64);  /* A: 0..31 → 64..95 */
                i++;
            } else if (c < 96) {
                EMIT(c - 32);  /* A: 32..95 → 0..63 */
                i++;
            } else {  /* c >= 96, lowercase / DEL not in A */
                int lowers_here = 0;
                while (i + (size_t)lowers_here < len &&
                       (unsigned char)data[i + lowers_here] >= 96 &&
                       (unsigned char)data[i + lowers_here] <= 127) {
                    lowers_here++;
                }
                if (lowers_here >= 2) {
                    EMIT(C128_CODE_B); subset = 'B';
                    continue;
                }
                EMIT(C128_SHIFT);
                EMIT(c - 32);  /* B-encoding of c */
                i++;
            }
        }
    }

    return (int)out;
#undef EMIT
}

/* Compute the mod-103 weighted checksum. The start code carries
 * weight 1; each subsequent code carries its 1-based position. */
static uint8_t code128_checksum(const uint8_t *codes, size_t n)
{
    long sum = codes[0];  /* start code, weight 1 */
    for (size_t k = 1; k < n; k++) {
        sum += (long)codes[k] * (long)k;
    }
    return (uint8_t)(sum % 103);
}

/* Render the encoded codes onto the target, applying foreground /
 * background colours and an optional human-readable text strip.
 * Returns 0 on success, -1 with a thrown PHP exception on failure.
 * Bars are emitted as <rect> elements via the SVG target. */
int fastchart_code128_render_to_target(fastchart_code128_obj *self,
                                        fastchart_target_t *t)
{
    fastchart_symbol_obj *base = (fastchart_symbol_obj *)self;

    if (!base->data || ZSTR_LEN(base->data) == 0) {
        zend_throw_error(NULL,
            "FastChart\\Code128 requires setData() before render");
        return -1;
    }

    /* Encode. */
    uint8_t codes[C128_MAX_CODES];
    char err[160] = {0};
    int n_codes = code128_encode(ZSTR_VAL(base->data), ZSTR_LEN(base->data),
                                 codes, C128_MAX_CODES, err, sizeof(err));
    if (n_codes < 0) {
        zend_throw_exception(zend_ce_value_error, err, 0);
        return -1;
    }

    /* Append checksum + stop. */
    if ((size_t)n_codes + 2 > C128_MAX_CODES) {
        zend_throw_error(NULL, "FastChart\\Code128: encoded code count overflow");
        return -1;
    }
    /* Compute checksum BEFORE incrementing n_codes; otherwise the
     * read of n_codes inside code128_checksum and the post-increment
     * at codes[n_codes++] cross a sequence point and the order of
     * evaluation is undefined (caught by -Wsequence-point). The
     * checksum weights treat the start code as position 0 with weight
     * 1 and counts every subsequent symbol's weight as its 1-based
     * index, which is what code128_checksum implements. */
    uint8_t check = code128_checksum(codes, (size_t)n_codes);
    codes[n_codes++] = check;
    codes[n_codes++] = C128_STOP;

    /* Total module count: every non-stop code contributes 11 modules,
     * the stop contributes 13. */
    int total_modules = (n_codes - 1) * 11 + 13;

    int W, H;
    fastchart_target_get_dims(t, &W, &H);

    /* Quiet zone & module pixel size.
     *
     *   - quiet_zone < 0  → "auto" (10 × narrowest bar = 10 × module_px).
     *   - quiet_zone >= 0 → explicit pixel margin on each side.
     *
     * Solve W = 2*quiet_px + total_modules * module_px:
     *   - auto:    W = 20*m + total*m        → m = W / (20 + total)
     *   - explicit: m = (W - 2*quiet_px) / total
     *
     * Reject when m < 1: the canvas is too small for the bar pattern
     * at any sane density. */
    int module_px;
    int quiet_px;
    if (base->quiet_zone < 0) {
        module_px = W / (20 + total_modules);
        if (module_px < 1) {
            zend_throw_exception(zend_ce_value_error,
                "FastChart\\Code128: canvas too narrow for the encoded bars "
                "+ default quiet zone (need at least 1 px per module)", 0);
            return -1;
        }
        quiet_px = 10 * module_px;
    } else {
        quiet_px = (int)base->quiet_zone;
        if (W <= 2 * quiet_px) {
            zend_throw_exception(zend_ce_value_error,
                "FastChart\\Code128: quiet zone consumes the entire canvas", 0);
            return -1;
        }
        module_px = (W - 2 * quiet_px) / total_modules;
        if (module_px < 1) {
            zend_throw_exception(zend_ce_value_error,
                "FastChart\\Code128: canvas too narrow for the encoded bars "
                "given the configured quiet zone", 0);
            return -1;
        }
    }

    /* Reserve a strip at the bottom for the human-readable text when
     * show_text is on AND a font is available. The default font path
     * is auto-detected at MINIT before any per-request open_basedir is
     * known, so re-check it here — a request that narrowed
     * open_basedir to exclude /usr/share/fonts must not be able to
     * open the auto-detected font any more. Silent fall-through
     * (warn=0) on rejection; bars-only render is still valid output.
     * Mirrors the chart-side check in fastchart_axis.c:check_font_path. */
    const char *text_font = NULL;
    int text_strip_h = 0;
    if (self->show_text && fastchart_default_font_path &&
        php_check_open_basedir_ex(fastchart_default_font_path,
                                  /*warn=*/0) == 0) {
        text_font = fastchart_default_font_path;
        text_strip_h = H / 5;
        if (text_strip_h < 14) text_strip_h = 14;
        if (text_strip_h > H / 2) text_strip_h = H / 2;
    }

    int bar_top = 2;
    int bar_bottom = H - 2 - text_strip_h;
    if (bar_bottom <= bar_top + 4) {
        zend_throw_exception(zend_ce_value_error,
            "FastChart\\Code128: canvas too short to render bars + text", 0);
        return -1;
    }

    /* Background fill via the shared helper — single source of truth
     * for the transparent_bg invariant. */
    fastchart_symbol_fill_background(base, t);

    int fg = fastchart_target_color_rgb(t, (int)base->fg_rgb);

    /* Walk codes, emitting bars. The first width is always a bar;
     * widths alternate space / bar / space / ... thereafter.
     *
     * Centre the bars on the canvas. With integer-rounded module_px,
     * bars + 2*quiet_px rarely consume the whole canvas; left-aligning
     * at quiet_px would dump all the slack on the right edge. The
     * `quiet_px` value is treated as the MINIMUM quiet zone — actual
     * quiet on each side is at least quiet_px and may be larger when
     * the canvas has slack from rounding. Mirrors the QR renderer's
     * centring convention in fastchart_qrcode.c. */
    int bars_px = total_modules * module_px;
    int x = (W - bars_px) / 2;
    int bar_h = bar_bottom - bar_top + 1;
    for (int k = 0; k < n_codes; k++) {
        const uint8_t *pattern;
        int n_widths;
        if (codes[k] == C128_STOP) {
            pattern = code128_stop_pattern;
            n_widths = 7;
        } else {
            pattern = code128_patterns[codes[k]];
            n_widths = 6;
        }
        int is_bar = 1;
        for (int w = 0; w < n_widths; w++) {
            int width_px = pattern[w] * module_px;
            if (is_bar && width_px > 0) {
                fastchart_target_rect(t, x, bar_top, width_px, bar_h,
                                      fg, /*fill=*/1, /*thickness=*/0);
            }
            x += width_px;
            is_bar = !is_bar;
        }
    }

    /* Human-readable text below the bars. Centred horizontally; the
     * raw input data is rendered as-is, control chars stripped to
     * spaces (otherwise FreeType may produce odd glyphs or truncate
     * at NUL — though setData already rejects NUL).
     *
     * Uses fastchart_text_draw with CENTER alignment so the SVG path
     * can rely on native text-anchor handling and the GD path can use
     * the same pre-measure-then-offset logic the rest of the chart
     * family uses. */
    if (text_font && text_strip_h > 0) {
        char buf[C128_MAX_INPUT + 1];
        size_t in_len = ZSTR_LEN(base->data);
        if (in_len > C128_MAX_INPUT) in_len = C128_MAX_INPUT;
        memcpy(buf, ZSTR_VAL(base->data), in_len);
        buf[in_len] = '\0';
        for (size_t k = 0; k < in_len; k++) {
            if ((unsigned char)buf[k] < 32 || (unsigned char)buf[k] > 126) {
                buf[k] = ' ';
            }
        }

        /* Font size targets ~55% of the strip height at 96 DPI; FT
         * renders at canvas DPI, so the physical glyph height is
         * pt * canvas_dpi/72. Scale pt down by 96/canvas_dpi so the
         * rendered text fits the strip regardless of resolution.
         * Without this, setDpi(200) inflates the rendered text past
         * the 1/5-of-H strip allocation and the `ty + 2 > H` clamp
         * below pulls the baseline back into the bars. Min 8pt. */
        int canvas_dpi = fastchart_target_get_dpi(t);
        if (canvas_dpi <= 0) canvas_dpi = 96;
        double pt = (double)text_strip_h * 0.55 * 96.0 / (double)canvas_dpi;
        if (pt < 8.0) pt = 8.0;

        /* Measure to position the baseline below the bars. Falls back
         * to a height proportional to pt when measurement fails. */
        int text_w = 0, text_h = 0;
        if (fastchart_text_measure(t, text_font, pt, buf,
                                   &text_w, &text_h, NULL, 0) != 0) {
            text_h = (int)(pt * 1.2 + 0.5);
        }
        int tx = W / 2;  /* centre anchor; CENTER align handles offset */
        /* Center the text vertically within the strip below the bars.
         * The previous +2 padding produced a visibly tight gap above
         * the text when text_h consumed most of text_strip_h; the
         * remaining slack was dumped below the text by accident. */
        int strip_top = bar_bottom + 1;
        int slack = text_strip_h - text_h;
        if (slack < 4) slack = 4;
        int top_pad = slack / 2;
        int ty = strip_top + top_pad + text_h;
        if (ty + 2 > H) ty = H - 2;
        (void)fastchart_text_draw(t, text_font, pt, fg, tx, ty,
                                  FASTCHART_ALIGN_CENTER, buf, NULL, 0);
        /* If the draw fails (font load issue), silently skip — bars
         * are still valid output. */
    }

    return 0;
}

