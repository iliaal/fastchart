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
#include "fastchart_target.h"
#include <plutosvg.h>
#include <plutovg.h>

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

/* Defined in fastchart_target.c — declared here to avoid pulling all
 * of fastchart_target.h's FT_Library + glyph-cache surface into this
 * file's public API surface. */
extern void fastchart_apply_text_overlays(void *plutovg_surface,
                                           int logical_w, int logical_h,
                                           const fastchart_text_overlay_t *overlays,
                                           int n_overlays);

#if defined(__x86_64__) || defined(_M_X64)
#  define FC_HAVE_X86_SIMD 1
#  include <immintrin.h>
#endif

/* inv_alpha LUT for un-premultiply: replaces the per-pixel integer
 * divide `(c * 255 + a/2) / a` with `(c * inv_alpha[a] + 0x8000) >> 16`.
 *
 *   inv_alpha[a] = round((255 << 16) / a)   for a = 1..255
 *   inv_alpha[0] = 0  (the a==0 branch short-circuits and never reads)
 *
 * Worst-case product: c_p ≤ a (premultiplication invariant), so
 *   c_p * inv_alpha[a] ≤ a * round(16711680/a) ≈ 16711680 < 2^31
 * — no 32-bit overflow.
 *
 * Idempotent init: filling the table is ~1 KB of integer divides at
 * MINIT (or first call). Cost is irrelevant; the table is process-wide
 * read-only data after init. */
static uint32_t fc_inv_alpha[256];
static int      fc_inv_alpha_ready = 0;

static void fc_init_inv_alpha(void)
{
	fc_inv_alpha[0] = 0;
	for (int a = 1; a < 256; a++) {
		/* +a/2 for round-to-nearest; matches the original
		 * (c * 255 + a/2) / a semantics. */
		fc_inv_alpha[a] = (uint32_t)((255u * 65536u + a / 2) / a);
	}
	fc_inv_alpha_ready = 1;
}

#ifdef FC_HAVE_X86_SIMD
/* SSSE3 shuffle table: BGRA -> RGBA per 32-bit lane. Each pixel's
 * bytes 0 and 2 swap; byte 1 (green) and byte 3 (alpha) stay put. */
static const int8_t fc_bgra_to_rgba_shuf[16] = {
	 2,  1,  0,  3,
	 6,  5,  4,  7,
	10,  9,  8, 11,
	14, 13, 12, 15
};

/* SSSE3 opaque-row fast path. Returns the number of pixels processed
 * via SIMD (always a multiple of 4); the caller handles the remainder
 * and any translucent-pixel fallback. Sets *any_translucent to 1 if
 * any pixel in the SIMD-processed prefix had a < 255.
 *
 * Compiled with target("ssse3") so we don't need a project-wide
 * -mssse3; runtime dispatch picks this path only when the CPU
 * supports it (see fc_cpu_has_ssse3). */
__attribute__((target("ssse3")))
static int fc_unpremul_row_ssse3(const unsigned char *src, unsigned char *dst,
                                  int n_pixels, int *any_translucent)
{
	__m128i shuf      = _mm_loadu_si128((const __m128i *)fc_bgra_to_rgba_shuf);
	__m128i alpha_msk = _mm_set1_epi32((int)0xFF000000u);
	int x = 0;
	int simd_pixels = n_pixels & ~3;  /* round down to multiple of 4 */
	for (; x < simd_pixels; x += 4) {
		__m128i bgra = _mm_loadu_si128((const __m128i *)(src + x * 4));
		__m128i alphas = _mm_and_si128(bgra, alpha_msk);
		__m128i all_ff = _mm_cmpeq_epi32(alphas, alpha_msk);
		if (_mm_movemask_epi8(all_ff) != 0xFFFF) {
			/* Not all four lanes are opaque; punt to scalar. */
			return x;
		}
		__m128i rgba = _mm_shuffle_epi8(bgra, shuf);
		_mm_storeu_si128((__m128i *)(dst + x * 4), rgba);
	}
	(void)any_translucent;  /* opaque-only fast path */
	return x;
}

static int fc_cpu_has_ssse3(void)
{
	static int cached = -1;
	if (cached < 0) {
		__builtin_cpu_init();
		cached = __builtin_cpu_supports("ssse3") ? 1 : 0;
	}
	return cached;
}
#endif  /* FC_HAVE_X86_SIMD */

/* Scalar tail: process pixels [x_start..n_pixels). Reads BGRA from
 * src+x*4, writes RGBA to dst+x*4. Sets *any_translucent if any pixel
 * has a < 255. */
static void fc_unpremul_row_scalar(const unsigned char *src,
                                    unsigned char *dst,
                                    int x_start, int n_pixels,
                                    int *any_translucent)
{
	for (int x = x_start; x < n_pixels; x++) {
		const unsigned char *p = src + x * 4;
		unsigned char       *q = dst + x * 4;
		unsigned char b = p[0];
		unsigned char g = p[1];
		unsigned char r = p[2];
		unsigned char a = p[3];
		if (a == 255) {
			q[0] = r; q[1] = g; q[2] = b; q[3] = 255;
		} else if (a == 0) {
			q[0] = q[1] = q[2] = q[3] = 0;
			*any_translucent = 1;
		} else {
			uint32_t inv = fc_inv_alpha[a];
			q[0] = (unsigned char)((r * inv + 0x8000u) >> 16);
			q[1] = (unsigned char)((g * inv + 0x8000u) >> 16);
			q[2] = (unsigned char)((b * inv + 0x8000u) >> 16);
			q[3] = a;
			*any_translucent = 1;
		}
	}
}

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

	/* Bound the float BEFORE the cast — (int)f is UB per C11 6.3.1.4
	 * when f is outside the representable int range. A pathological
	 * SVG with width="1e10" would otherwise produce INT_MIN on x86
	 * (happens to fail the iw <= 0 check) or a trap on other ABIs.
	 *
	 * We use INT_MAX (not FC_IMAGE_MAX_DIM) as the guard so the
	 * downstream per-axis cap check at the caller still fires with
	 * the user-friendly "exceed cap" message for normal-but-too-
	 * large dimensions like width=50000. This bound is strictly for
	 * UB avoidance on truly absurd inputs. */
	if (w > (float)INT_MAX || h > (float)INT_MAX) return -1;

	int iw = (int)(w + 0.5f);
	int ih = (int)(h + 0.5f);
	if (iw <= 0 || ih <= 0) return -1;

	*out_w = iw;
	*out_h = ih;
	return 0;
}

/* Render an already-loaded plutosvg document into pix. Owns the
 * surface (destroys it); does NOT own the document. Returns 0 on
 * success, -1 on rasterize failure.
 *
 * If overlays/n_overlays is set, applies deferred text overlays
 * (opt #7) to the surface between plutosvg_render and un-premultiply.
 * logical_w/logical_h are the SVG document's intrinsic viewport
 * dimensions used to compute the logical->physical scale for text
 * positioning. */
static int fastchart_rasterize_doc(plutosvg_document_t *doc,
                                    int target_w, int target_h,
                                    int logical_w, int logical_h,
                                    const fastchart_text_overlay_t *overlays,
                                    int n_overlays,
                                    fastchart_pixels_t *pix)
{
	plutovg_surface_t *surf =
	    plutosvg_document_render_to_surface(doc, NULL, target_w, target_h,
	                                        NULL, NULL, NULL);
	if (!surf) return -1;

	if (n_overlays > 0 && overlays) {
		fastchart_apply_text_overlays(surf, logical_w, logical_h,
		                               overlays, n_overlays);
	}

	pix->w = target_w;
	pix->h = target_h;
	pix->has_alpha = 1;

	int sw = plutovg_surface_get_width(surf);
	int sh = plutovg_surface_get_height(surf);
	if (sw != target_w || sh != target_h) {
		pix->w = sw;
		pix->h = sh;
	}

	const unsigned char *src = plutovg_surface_get_data(surf);
	int                  stride = plutovg_surface_get_stride(surf);

	size_t bytes = (size_t)pix->w * pix->h * 4;
	pix->rgba = emalloc(bytes);

	if (!fc_inv_alpha_ready) fc_init_inv_alpha();

	/* Two-stage un-premultiply:
	 *   - opaque-row SIMD shuffle for runs of all-FF alpha (fastchart's
	 *     usual case — chart backgrounds and bar fills are opaque)
	 *   - scalar+LUT for translucent pixels and the row remainder
	 *
	 * The any_translucent flag tracks whether any pixel needed the
	 * un-premultiply path. If the whole surface is opaque, the encoders
	 * skip alpha entirely (see pix->has_alpha = 0 below) — smaller PNG
	 * files, faster JPEG/WebP encode, no alpha plane in WebP. */
	int any_translucent = 0;
#ifdef FC_HAVE_X86_SIMD
	int use_ssse3 = fc_cpu_has_ssse3();
#endif
	for (int y = 0; y < pix->h; y++) {
		const unsigned char *row = src + y * stride;
		unsigned char       *dst = pix->rgba + (size_t)y * pix->w * 4;
		int x = 0;
#ifdef FC_HAVE_X86_SIMD
		if (use_ssse3) {
			x = fc_unpremul_row_ssse3(row, dst, pix->w, &any_translucent);
		}
#endif
		fc_unpremul_row_scalar(row, dst, x, pix->w, &any_translucent);
	}

	pix->has_alpha = any_translucent ? 1 : 0;

	plutovg_surface_destroy(surf);
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

	int rc = fastchart_rasterize_doc(doc, target_w, target_h,
	                                  0, 0, NULL, 0, pix);
	plutosvg_document_destroy(doc);
	return rc;
}

int fastchart_rasterize_svg_with_text(const char *svg, size_t svg_len,
                                       int target_w, int target_h,
                                       int logical_w, int logical_h,
                                       const fastchart_text_overlay_t *overlays,
                                       int n_overlays,
                                       fastchart_pixels_t *pix)
{
	pix->rgba = NULL;
	pix->w = target_w;
	pix->h = target_h;
	pix->has_alpha = 1;

	plutosvg_document_t *doc =
	    plutosvg_document_load_from_data(svg, (int)svg_len, -1, -1,
	                                     NULL, NULL);
	if (!doc) return -1;

	int rc = fastchart_rasterize_doc(doc, target_w, target_h,
	                                  logical_w, logical_h,
	                                  overlays, n_overlays, pix);
	plutosvg_document_destroy(doc);
	return rc;
}

/* Single-pass: parse → read intrinsic dims → cap-check → rasterize.
 * Avoids the double parse the previous (get_intrinsic_dims +
 * rasterize_svg) sequence imposed on the Chart::svgToPng/Jpeg/Webp
 * static methods. */
int fastchart_rasterize_svg_with_dims(const char *svg, size_t svg_len,
                                       int max_dim, long long max_pixels,
                                       fastchart_pixels_t *pix,
                                       int *out_w, int *out_h)
{
	pix->rgba = NULL;
	pix->w = 0;
	pix->h = 0;
	pix->has_alpha = 1;
	if (out_w) *out_w = 0;
	if (out_h) *out_h = 0;

	plutosvg_document_t *doc =
	    plutosvg_document_load_from_data(svg, (int)svg_len, -1, -1,
	                                     NULL, NULL);
	if (!doc) return -1;

	float w = plutosvg_document_get_width(doc);
	float h = plutosvg_document_get_height(doc);
	if (!isfinite(w) || !isfinite(h) || w <= 0 || h <= 0
	    || w > (float)INT_MAX || h > (float)INT_MAX) {
		plutosvg_document_destroy(doc);
		return -2;
	}
	int iw = (int)(w + 0.5f);
	int ih = (int)(h + 0.5f);
	if (iw <= 0 || ih <= 0) {
		plutosvg_document_destroy(doc);
		return -2;
	}
	if (out_w) *out_w = iw;
	if (out_h) *out_h = ih;

	if (iw > max_dim || ih > max_dim
	    || (long long)iw * ih > max_pixels) {
		plutosvg_document_destroy(doc);
		return -3;
	}

	int rc = fastchart_rasterize_doc(doc, iw, ih, 0, 0, NULL, 0, pix);
	plutosvg_document_destroy(doc);
	return (rc == 0) ? 0 : -4;
}
