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

  Raster encoder layer. Takes a plain RGBA pixel buffer (top-down,
  pre-multiplied or straight alpha — caller's choice) and produces a
  PHP smart_str of encoded bytes. Replaces the libgd-based
  fastchart_encode_image() helper.

  Each of libpng / libjpeg-turbo / libwebp is independently probed
  by config.m4. Missing libs drop their corresponding output format;
  the encode entry point compiles into a stub that returns -2 and
  the caller throws a clear error at the PHP boundary. FreeType is
  unconditionally required (text rendering depends on it).
*/

#ifndef FASTCHART_ENCODER_H
#define FASTCHART_ENCODER_H

#include "php.h"
#include "zend_smart_str.h"
#include <stdint.h>

typedef struct {
	uint8_t *rgba;        /* w * h * 4 bytes, row-major, RGBA, top-down */
	int      w;
	int      h;
	int      has_alpha;   /* 0 = opaque (RGB output OK); 1 = real alpha present */
	int      dpi;         /* 0 = don't write density metadata; else stamps pHYs/density */
} fastchart_pixels_t;

/* Allocate an empty pixel buffer. rgba is set to NULL and must be
 * filled by the rasterizer before encoding. */
void fastchart_pixels_init(fastchart_pixels_t *pix, int w, int h);

/* Free the pixel buffer; safe to call on uninitialised / zeroed pix. */
void fastchart_pixels_release(fastchart_pixels_t *pix);

/* Encode pix into PNG bytes. Returns 0 on success, -1 on encode
 * failure, -2 if libpng is not compiled in. */
int fastchart_encode_png(smart_str *out, const fastchart_pixels_t *pix);

/* Encode pix into JPEG bytes. quality is 1..100; clamped if out of
 * range. -2 if libjpeg-turbo is not compiled in. */
int fastchart_encode_jpeg(smart_str *out, const fastchart_pixels_t *pix,
                          int quality);

/* WebP encoding mode. See setWebpMode() in fastchart.stub.php for the
 * caller-facing description. DRAWING is the default and is tuned for
 * chart-shaped content (flat fills, sharp edges). */
#define FASTCHART_WEBP_DRAWING   0  /* WEBP_PRESET_DRAWING, method=2 */
#define FASTCHART_WEBP_PHOTO     1  /* WEBP_PRESET_PHOTO,   method=4 */
#define FASTCHART_WEBP_LOSSLESS  2  /* lossless = 1,        method=6 */
#define FASTCHART_WEBP_FAST      3  /* WEBP_PRESET_DRAWING, method=0 */

/* Encode pix into WebP bytes. mode is one of FASTCHART_WEBP_*; in
 * LOSSLESS mode `quality` is ignored for perceptual quality and the
 * encoder uses lossless compression. -2 if libwebp is not compiled in. */
int fastchart_encode_webp(smart_str *out, const fastchart_pixels_t *pix,
                          int quality, int mode);

/* Build-time availability + version strings. The "_version" helpers
 * return NULL when the lib isn't compiled in. */
int         fastchart_have_libpng(void);
int         fastchart_have_libjpeg(void);
int         fastchart_have_libwebp(void);
const char *fastchart_libpng_version(void);
const char *fastchart_libjpeg_version(void);
const char *fastchart_libwebp_version(void);

#endif /* FASTCHART_ENCODER_H */
