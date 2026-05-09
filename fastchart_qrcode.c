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

  QR Code (ISO/IEC 18004) implementation. Thin wrapper around the
  vendored nayuki qrcodegen encoder (vendor/qrcodegen/), which
  produces a boolean module grid; this file maps that grid to a
  filled-rectangle render on the supplied gdImagePtr.

  ECC levels (FastChart\QrCode::ECC_L / M / Q / H) map 1:1 onto
  qrcodegen_Ecc_LOW / MEDIUM / QUARTILE / HIGH so the cast is direct.
  setMinVersion / setMaxVersion let callers pin a specific symbol
  size; the encoder picks the smallest version that fits the data
  within the requested range.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "php.h"
#include "Zend/zend_exceptions.h"

#include "php_fastchart.h"
#include "qrcodegen.h"

int fastchart_qrcode_render_to_image(fastchart_qrcode_obj *self, gdImagePtr im)
{
    fastchart_symbol_obj *base = (fastchart_symbol_obj *)self;

    if (!base->data || ZSTR_LEN(base->data) == 0) {
        zend_throw_error(NULL,
            "FastChart\\QrCode requires setData() before render");
        return -1;
    }

    /* Cross-bound min/max version check at render time rather than
     * setter time. Setting just one of the two leaves a transient
     * out-of-order state (e.g. setMinVersion(40) before
     * setMaxVersion(40)); deferring the check until render keeps the
     * setter API order-independent. nayuki's encoder also rejects
     * range inversion internally, but a clean PHP exception with
     * specific values is more useful than a generic encode failure. */
    if (self->min_version > self->max_version) {
        zend_throw_exception_ex(zend_ce_value_error, 0,
            "FastChart\\QrCode: setMinVersion(%lld) > setMaxVersion(%lld); "
            "the version range must be non-empty",
            (long long)self->min_version, (long long)self->max_version);
        return -1;
    }

    /* Stack-allocate the encoder's two scratch buffers. Each is sized
     * for the largest QR version (40, 177×177 modules); the macro
     * expands to 3918 at the time of writing, so two of them = ~8 KB
     * stack. Well within both the default Linux per-thread stack and
     * the smaller stacks ZTS PHP uses. Keeping them on the stack
     * avoids a per-render emalloc/efree pair. */
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempbuf[qrcodegen_BUFFER_LEN_MAX];

    bool ok = qrcodegen_encodeText(
        ZSTR_VAL(base->data),
        tempbuf,
        qrcode,
        (enum qrcodegen_Ecc)self->ecc,
        (int)self->min_version,
        (int)self->max_version,
        qrcodegen_Mask_AUTO,
        /*boostEcl=*/true);

    if (!ok) {
        zend_throw_exception_ex(zend_ce_value_error, 0,
            "FastChart\\QrCode: data does not fit within the requested "
            "version range (min=%lld, max=%lld) at ECC level %lld; "
            "increase setMaxVersion() or relax setEcc()",
            (long long)self->min_version, (long long)self->max_version,
            (long long)self->ecc);
        return -1;
    }

    int N = qrcodegen_getSize(qrcode);  /* 21..177 modules per side */

    int W = gdImageSX(im);
    int H = gdImageSY(im);

    /* Quiet zone: spec says 4 modules around every QR symbol. The
     * setQuietZone() unit for QrCode is modules (per the stub). -1
     * sentinels "use class default".
     *
     * The Symbol-level setQuietZone validator accepts up to 4096
     * (a generic upper cap that fits any reasonable Code128 pixel
     * margin) but for QR the unit is modules and 256 modules of
     * quiet zone already dwarfs any conceivable symbol size. Reject
     * anything above 256 with a specific exception rather than
     * silently clamping — silent clamps surprise callers who
     * configured a value the setter accepted. */
    int quiet_modules;
    if (base->quiet_zone < 0) {
        quiet_modules = 4;
    } else if (base->quiet_zone > 256) {
        zend_throw_exception_ex(zend_ce_value_error, 0,
            "FastChart\\QrCode: quiet zone %lld modules exceeds the QR "
            "maximum of 256 modules",
            (long long)base->quiet_zone);
        return -1;
    } else {
        quiet_modules = (int)base->quiet_zone;
    }

    int total_modules = N + 2 * quiet_modules;

    /* Module pixel size: the symbol must fit in the SHORTER canvas
     * dimension so it stays square. Floor division — slack pixels
     * land in the centring offsets below. */
    int module_px_w = W / total_modules;
    int module_px_h = H / total_modules;
    int module_px = module_px_w < module_px_h ? module_px_w : module_px_h;
    if (module_px < 1) {
        zend_throw_exception_ex(zend_ce_value_error, 0,
            "FastChart\\QrCode: canvas %dx%d too small for a %dx%d-module "
            "symbol + %d-module quiet zone (need at least 1 px per module)",
            W, H, N, N, quiet_modules);
        return -1;
    }

    /* Centre the symbol on the canvas. The symbol occupies
     * total_modules * module_px pixels each side; the rest is
     * centring slack inside the quiet zone (so the effective quiet
     * zone is at LEAST quiet_modules * module_px on every side). */
    int symbol_px = total_modules * module_px;
    int origin_x = (W - symbol_px) / 2 + quiet_modules * module_px;
    int origin_y = (H - symbol_px) / 2 + quiet_modules * module_px;

    /* Background fill via the shared helper — single source of truth
     * for the transparent_bg invariant. */
    fastchart_symbol_fill_background(base, im);

    int fg = gdImageColorAllocate(im,
        (int)((base->fg_rgb >> 16) & 0xFF),
        (int)((base->fg_rgb >> 8)  & 0xFF),
        (int)(base->fg_rgb & 0xFF));

    /* Walk the module grid. Adjacent dark modules form runs along the
     * x-axis; coalesce them into single gdImageFilledRectangle calls
     * to reduce the libgd call count by ~3-5x on typical QR symbols
     * (versions 1-10), which speeds the render and shrinks PNG
     * encode work as a side effect. The visible output is identical. */
    for (int y = 0; y < N; y++) {
        int x = 0;
        while (x < N) {
            if (!qrcodegen_getModule(qrcode, x, y)) {
                x++;
                continue;
            }
            int run_start = x;
            while (x < N && qrcodegen_getModule(qrcode, x, y)) x++;
            int run_end = x - 1;  /* inclusive */
            int x0 = origin_x + run_start * module_px;
            int x1 = origin_x + (run_end + 1) * module_px - 1;
            int y0 = origin_y + y * module_px;
            int y1 = origin_y + (y + 1) * module_px - 1;
            gdImageFilledRectangle(im, x0, y0, x1, y1, fg);
        }
    }

    return 0;
}
