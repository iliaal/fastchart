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

  SVG -> RGBA buffer via plutosvg (SVG parser) + plutovg (rasterizer).
*/

#include "fastchart_rasterize.h"
#include <plutosvg.h>
#include <plutovg.h>

#include <math.h>
#include <string.h>

int fastchart_svg_get_intrinsic_dims(const char *svg, size_t svg_len,
                                     int *out_w, int *out_h)
{
	plutosvg_document_t *doc =
	    plutosvg_document_load_from_data(svg, (int)svg_len, -1, -1,
	                                     NULL, NULL);
	if (!doc) return -1;

	float w = plutosvg_document_get_width(doc);
	float h = plutosvg_document_get_height(doc);
	plutosvg_document_destroy(doc);

	/* plutosvg returns -1 when the document declares percentage
	 * dimensions without a container, or 0 when it has neither
	 * width/height nor viewBox. Either is "unresolvable" for our
	 * purposes — fastchart doesn't carry an outer viewport. */
	if (!isfinite(w) || !isfinite(h) || w <= 0 || h <= 0) return -1;

	int iw = (int)(w + 0.5f);
	int ih = (int)(h + 0.5f);
	if (iw <= 0 || ih <= 0) return -1;

	*out_w = iw;
	*out_h = ih;
	return 0;
}

int fastchart_rasterize_svg(const char *svg, size_t svg_len,
                            int target_w, int target_h,
                            fastchart_pixels_t *pix)
{
	pix->rgba = NULL;
	pix->w = target_w;
	pix->h = target_h;
	pix->has_alpha = 1;  /* plutovg returns premultiplied BGRA */

	plutosvg_document_t *doc =
	    plutosvg_document_load_from_data(svg, (int)svg_len, -1, -1,
	                                     NULL, NULL);
	if (!doc) return -1;

	plutovg_surface_t *surf =
	    plutosvg_document_render_to_surface(doc, NULL, target_w, target_h,
	                                        NULL, NULL, NULL);
	plutosvg_document_destroy(doc);
	if (!surf) return -1;

	int sw = plutovg_surface_get_width(surf);
	int sh = plutovg_surface_get_height(surf);
	if (sw != target_w || sh != target_h) {
		/* plutovg honored its own dimension preference; accept what
		 * it returned but keep dims consistent. */
		pix->w = sw;
		pix->h = sh;
	}

	const unsigned char *src = plutovg_surface_get_data(surf);
	int                  stride = plutovg_surface_get_stride(surf);

	size_t bytes = (size_t)pix->w * pix->h * 4;
	pix->rgba = emalloc(bytes);

	/* plutovg surface format is premultiplied BGRA (32-bit native).
	 * Convert to straight RGBA for the encoders: divide RGB by alpha,
	 * swap B<->R. Encoders flatten alpha appropriately. */
	for (int y = 0; y < pix->h; y++) {
		const unsigned char *row = src + y * stride;
		unsigned char       *dst = pix->rgba + (size_t)y * pix->w * 4;
		for (int x = 0; x < pix->w; x++) {
			unsigned char b = row[0];
			unsigned char g = row[1];
			unsigned char r = row[2];
			unsigned char a = row[3];
			if (a == 0) {
				dst[0] = dst[1] = dst[2] = 0;
				dst[3] = 0;
			} else if (a == 255) {
				dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = 255;
			} else {
				/* un-premultiply: c = c_p * 255 / a */
				dst[0] = (unsigned char)((r * 255 + a / 2) / a);
				dst[1] = (unsigned char)((g * 255 + a / 2) / a);
				dst[2] = (unsigned char)((b * 255 + a / 2) / a);
				dst[3] = a;
			}
			row += 4;
			dst += 4;
		}
	}

	plutovg_surface_destroy(surf);
	return 0;
}
