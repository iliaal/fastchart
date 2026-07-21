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

  v1.0: the former fastchart_encode_image() helper retired. Raster
  outputs go through fastchart_encoder.c (PNG/JPG/WebP via libpng /
  libjpeg-turbo / libwebp) after fastchart_rasterize.c rasterizes the
  SVG document via plutovg.
*/

#ifndef FASTCHART_RENDER_HELPERS_H
#define FASTCHART_RENDER_HELPERS_H

#include "php.h"
#include "main/php_streams.h"
#include "Zend/zend_smart_str.h"

struct fastchart_target;

typedef int (*fastchart_svg_render_cb)(void *object,
	zend_class_entry *ce, struct fastchart_target *target);

/* Build one SVG envelope around a Chart or Symbol render callback.
 * Full documents include the XML/SVG wrapper; fragments contain only
 * the requested classed group. On failure the target and output are
 * released here. On success the caller owns out->s. */
int fastchart_build_svg(smart_str *out, int width, int height, int dpi,
	int text_mode, int fragment_only, const char *group_class,
	zend_string *id_prefix, fastchart_svg_render_cb render,
	void *object, zend_class_entry *ce);

/* Resolve the physical (allocated) canvas dimensions from logical
 * width/height + DPI. Honours the 16384px / 64M-pixel cap. Throws a
 * ValueError and returns -1 if the resolved dims exceed the cap;
 * returns 0 with *out_w / *out_h populated on success. */
int fastchart_resolve_canvas_dims(zend_long width, zend_long height,
                                  zend_long dpi,
                                  int *out_w, int *out_h);

/* Return the missing encoder library name for format 0..2, or NULL
 * when the requested encoder is available. */
const char *fastchart_missing_encoder_lib(int format);

/* Map the file extension at the tail of `path` to one of the encoder
 * format ints (0..4). Returns -1 when no recognised extension is
 * present. ASCII-fold; locale-independent.
 * Format codes: 0=PNG, 1=JPEG, 2=WebP, 3=GIF (rejected), 4=AVIF (rejected). */
int fastchart_format_from_path(const char *path, size_t len);

/* Case-insensitive ASCII check for a `.svg` tail using the same
 * bounded last-extension parsing as fastchart_format_from_path(). */
int fastchart_path_ends_with_svg(const char *path, size_t len);

typedef struct {
	php_stream *stream;
	zend_string *path;
	zend_string *tmp_path;
	const char *where;
	int final_mode;
} fastchart_atomic_file_t;

/* Create a mode-0600 sibling temporary file for an eventual atomic rename.
 * The caller must commit or abort every successful open. */
int fastchart_atomic_file_open(fastchart_atomic_file_t *file,
	zend_string *path, const char *where);
void fastchart_atomic_file_abort(fastchart_atomic_file_t *file);
int fastchart_atomic_file_commit(fastchart_atomic_file_t *file,
	size_t written, zend_long *written_out);

/* Write `payload` to `path` through the Zend stream layer. `where`
 * prefixes the open-failure message, e.g. "FastChart\\Chart::renderToFile()".
 * On success, stores the byte count in written_out when non-NULL. */
int fastchart_write_zstr_to_file(zend_string *path, zend_string *payload,
                                 const char *where,
                                 zend_long *written_out);

#endif /* FASTCHART_RENDER_HELPERS_H */
