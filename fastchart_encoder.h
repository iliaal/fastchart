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

  libpng / libjpeg-turbo / libwebp are linked directly via pkg-config
  in config.m4. fastchart_pixels_t owns the RGBA buffer and frees it
  in fastchart_pixels_release().
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
} fastchart_pixels_t;

/* Allocate an empty pixel buffer. rgba is set to NULL and must be
 * filled by the rasterizer before encoding. */
void fastchart_pixels_init(fastchart_pixels_t *pix, int w, int h);

/* Free the pixel buffer; safe to call on uninitialised / zeroed pix. */
void fastchart_pixels_release(fastchart_pixels_t *pix);

/* Encode pix into PNG bytes. Returns 0 on success, -1 on failure.
 * On success, the bytes are appended to out (smart_str must be
 * initialised by caller). */
int fastchart_encode_png(smart_str *out, const fastchart_pixels_t *pix);

/* Encode pix into JPEG bytes. quality is 1..100; clamped if out of
 * range. Uses libjpeg-turbo with optimize_coding=TRUE, 4:2:0
 * subsampling, non-progressive — matches the q88 reference
 * established during the plutovg eval. */
int fastchart_encode_jpeg(smart_str *out, const fastchart_pixels_t *pix,
                          int quality);

/* Encode pix into WebP bytes (lossy). quality is 1..100; clamped if
 * out of range. Uses libwebp's WebPEncodeRGBA. */
int fastchart_encode_webp(smart_str *out, const fastchart_pixels_t *pix,
                          int quality);

#endif /* FASTCHART_ENCODER_H */
