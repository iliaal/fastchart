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

  Render-pipeline helpers shared between the Chart family (renderPng /
  renderJpeg / ... in fastchart.c) and the Symbol family (renderPng /
  ... in fastchart_symbol.c). The canvas dimension cap policy and the
  format dispatch table live here; both families go through these so a
  future cap change lands in one place.

  The dim resolver takes scalars (width / height / dpi) rather than a
  base-struct pointer because Chart and Symbol have separate base
  layouts (FASTCHART_BASE_FIELDS vs FASTCHART_SYMBOL_BASE_FIELDS) with
  no common parent type.

  v1.0: the libgd-based fastchart_encode_image() helper retired. Raster
  outputs go through fastchart_encoder.c (PNG/JPG/WebP via libpng /
  libjpeg-turbo / libwebp) after fastchart_rasterize.c rasterizes the
  SVG document via plutovg.
*/

#ifndef FASTCHART_RENDER_HELPERS_H
#define FASTCHART_RENDER_HELPERS_H

#include "php.h"

/* Resolve the physical (allocated) canvas dimensions from logical
 * width/height + DPI. Honours the 16384px / 64M-pixel cap. Throws a
 * ValueError and returns -1 if the resolved dims exceed the cap;
 * returns 0 with *out_w / *out_h populated on success. */
int fastchart_resolve_canvas_dims(zend_long width, zend_long height,
                                  zend_long dpi,
                                  int *out_w, int *out_h);

/* Map the file extension at the tail of `path` to one of the encoder
 * format ints (0..4). Returns -1 when no recognised extension is
 * present. ASCII-fold; locale-independent.
 * Format codes: 0=PNG, 1=JPEG, 2=WebP, 3=GIF (rejected), 4=AVIF (rejected). */
int fastchart_format_from_path(const char *path, size_t len);

#endif /* FASTCHART_RENDER_HELPERS_H */
