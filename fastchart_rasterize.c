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
#include "php_fastchart.h"
#include <plutosvg.h>
#include <plutovg.h>

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

/* Decoded source-image surfaces plutosvg retains for one document.
 * fastchart.max_image_cache_bytes can lower it but never raise it past
 * the built-in ceiling; a non-positive or oversized setting means "use
 * the built-in". Lowering only costs repeat decodes. */
#define FC_IMAGE_CACHE_MAX_BYTES ((size_t)64 * 1024 * 1024)

static size_t fastchart_image_cache_limit(void)
{
	zend_long configured = FASTCHART_G(max_image_cache_bytes);
	if (configured > 0
	    && (size_t)configured < FC_IMAGE_CACHE_MAX_BYTES) {
		return (size_t)configured;
	}
	return FC_IMAGE_CACHE_MAX_BYTES;
}

/* x86 SSSE3 worker fns carry __attribute__((target("ssse3"))); runtime
 * dispatch detects the feature via raw CPUID (__get_cpuid from
 * <cpuid.h>), NOT __builtin_cpu_supports. The builtin pulls in libgcc's
 * __cpu_model / __cpu_indicator_init, which are absent or unexported in
 * fully-static / musl / zig-cc builds (e.g. static-php-cli): the shared
 * object then fails to dlopen with an undefined-symbol error. CPUID is
 * emitted inline and needs no runtime support symbols. MSVC (no __GNUC__)
 * falls through to scalar. AArch64 NEON is baseline, no dispatch. */
#if (defined(__x86_64__) || defined(_M_X64)) && defined(__GNUC__)
#  define FC_HAVE_X86_SIMD 1
#  include <immintrin.h>
#  include <cpuid.h>
#endif
#if defined(__aarch64__) && (defined(__ARM_NEON) || defined(__ARM_NEON__))
#  define FC_HAVE_ARM_NEON 1
#  include <arm_neon.h>
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
static int fc_cpu_has_ssse3(void);
#endif

/* Fill the LUT once at module load. After this, fc_inv_alpha_ready is
 * already 1 before any request thread runs, so the lazy first-call branch
 * in fastchart_rasterize_doc is never taken concurrently — closing the
 * ZTS data race on the unsynchronised ready flag. The SSSE3 capability
 * cache is prewarmed here for the same reason. */
void fastchart_rasterize_init(void)
{
	fc_init_inv_alpha();
#ifdef FC_HAVE_X86_SIMD
	(void)fc_cpu_has_ssse3();
#endif
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
		unsigned int eax, ebx, ecx, edx;
		cached = (__get_cpuid(1, &eax, &ebx, &ecx, &edx)
		          && (ecx & bit_SSSE3)) ? 1 : 0;
	}
	return cached;
}
#endif  /* FC_HAVE_X86_SIMD */

#ifdef FC_HAVE_ARM_NEON
static int fc_unpremul_row_neon(const unsigned char *src, unsigned char *dst,
                                 int n_pixels, int *any_translucent)
{
	int x = 0;
	int simd_pixels = n_pixels & ~15;  /* round down to multiple of 16 */
	for (; x < simd_pixels; x += 16) {
		uint8x16x4_t bgra = vld4q_u8(src + (size_t)x * 4);
		if (vminvq_u8(bgra.val[3]) != 255) {
			return x;
		}

		uint8x16x4_t rgba;
		rgba.val[0] = bgra.val[2];
		rgba.val[1] = bgra.val[1];
		rgba.val[2] = bgra.val[0];
		rgba.val[3] = bgra.val[3];
		vst4q_u8(dst + (size_t)x * 4, rgba);
	}
	(void)any_translucent;  /* opaque-only fast path */
	return x;
}
#endif  /* FC_HAVE_ARM_NEON */

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

/* Draw an already-loaded plutosvg document into `frame`, a caller-owned
 * buffer with capacity target_w * target_h * 4. Owns the canvas and the
 * wrapper surface; owns neither the document nor the buffer.
 *
 * `frame` MUST be allocated BEFORE any vendor (malloc'd) state is
 * created. The wrapper and canvas are malloc-backed and explicitly
 * released from the bailout path. This function performs no Zend
 * allocation.
 *
 * The un-premultiply pass deliberately lives in the caller rather than
 * here: gcc's -Wclobbered covers every non-volatile local in a function
 * that calls setjmp, and the temporaries inlined NEON intrinsics declare
 * trip it on aarch64. Keeping the setjmp in its own function leaves the
 * SIMD loop in a scope the diagnostic never inspects. */
static int fastchart_render_doc_to_frame(plutosvg_document_t *doc,
                                          int target_w, int target_h,
                                          unsigned char *frame)
{
	plutovg_surface_t * volatile surf = NULL;
	plutovg_canvas_t * volatile canvas = NULL;
	volatile int rendered = 0;
	float doc_w = plutosvg_document_get_width(doc);
	float doc_h = plutosvg_document_get_height(doc);

	if (!isfinite(doc_w) || !isfinite(doc_h) || doc_w <= 0 || doc_h <= 0) {
		return -1;
	}

	memset(frame, 0, (size_t)target_w * (size_t)target_h * 4);
	zend_try {
		surf = plutovg_surface_create_for_data(frame, target_w,
		                                       target_h, target_w * 4);
		if (surf) {
			canvas = plutovg_canvas_create((plutovg_surface_t *)surf);
		}
		if (canvas) {
			plutovg_canvas_scale((plutovg_canvas_t *)canvas,
			                     target_w / doc_w, target_h / doc_h);
			if (plutosvg_document_render(doc, NULL,
			        (plutovg_canvas_t *)canvas, NULL, NULL, NULL)) {
				rendered = 1;
			}
		}
		if (canvas) {
			plutovg_canvas_destroy((plutovg_canvas_t *)canvas);
			canvas = NULL;
		}
		if (surf) {
			plutovg_surface_destroy((plutovg_surface_t *)surf);
			surf = NULL;
		}
	} zend_catch {
		if (canvas) plutovg_canvas_destroy((plutovg_canvas_t *)canvas);
		if (surf) plutovg_surface_destroy((plutovg_surface_t *)surf);
		zend_bailout();
	} zend_end_try();
	return rendered ? 0 : -1;
}

/* Render an already-loaded plutosvg document into pix and convert the
 * frame from plutovg's pre-multiplied BGRA to straight RGBA in place.
 * Returns 0 on success, -1 on rasterize failure. See
 * fastchart_render_doc_to_frame for the pix->rgba pre-allocation
 * contract. */
static int fastchart_rasterize_doc(plutosvg_document_t *doc,
                                    int target_w, int target_h,
                                    fastchart_pixels_t *pix)
{
	if (fastchart_render_doc_to_frame(doc, target_w, target_h,
			pix->rgba) != 0) {
		return -1;
	}

	pix->w = target_w;
	pix->h = target_h;

	if (!fc_inv_alpha_ready) fc_init_inv_alpha();

	/* Two-stage un-premultiply:
	 *   - opaque-row SIMD shuffle for runs of all-FF alpha
	 *   - scalar+LUT for translucent pixels and row tails */
	int any_translucent = 0;
#ifdef FC_HAVE_X86_SIMD
	int use_ssse3 = fc_cpu_has_ssse3();
#endif
	for (int y = 0; y < pix->h; y++) {
		const unsigned char *row = pix->rgba + (size_t)y * pix->w * 4;
		unsigned char *dst = pix->rgba + (size_t)y * pix->w * 4;
		int x = 0;
#ifdef FC_HAVE_ARM_NEON
		x = fc_unpremul_row_neon(row, dst, pix->w, &any_translucent);
#elif defined(FC_HAVE_X86_SIMD)
		if (use_ssse3) {
			x = fc_unpremul_row_ssse3(row, dst, pix->w, &any_translucent);
		}
#endif
		fc_unpremul_row_scalar(row, dst, x, pix->w, &any_translucent);
	}
	pix->has_alpha = any_translucent ? 1 : 0;
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
	if (svg_len > (size_t)INT_MAX) return -1;
	if (target_w <= 0 || target_h <= 0) return -1;

	/* Destination first — see fastchart_rasterize_doc's no-Zend-alloc
	 * contract for the vendor-state window. */
	pix->rgba = safe_emalloc((size_t)target_w * (size_t)target_h, 4, 0);

	plutosvg_document_t *doc =
	    plutosvg_document_load_from_data(svg, (int)svg_len, -1, -1,
	                                     NULL, NULL);
	if (!doc) {
		efree(pix->rgba);
		pix->rgba = NULL;
		return -1;
	}
	plutosvg_document_set_image_cache_limit(doc,
		fastchart_image_cache_limit());

	int rc;
	zend_try {
		rc = fastchart_rasterize_doc(doc, target_w, target_h, pix);
	} zend_catch {
		plutosvg_document_destroy(doc);
		if (pix->rgba) {
			efree(pix->rgba);
			pix->rgba = NULL;
		}
		zend_bailout();
	} zend_end_try();
	plutosvg_document_destroy(doc);
	if (rc != 0) {
		efree(pix->rgba);
		pix->rgba = NULL;
	}
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
	if (svg_len > (size_t)INT_MAX) return -2;

	plutosvg_document_t *doc =
	    plutosvg_document_load_from_data(svg, (int)svg_len, -1, -1,
	                                     NULL, NULL);
	if (!doc) return -1;
	plutosvg_document_set_image_cache_limit(doc,
		fastchart_image_cache_limit());

	float w = plutosvg_document_get_width(doc);
	float h = plutosvg_document_get_height(doc);
	/* >= : (float)INT_MAX rounds up to 2^31, which `>` would admit
	 * straight into the UB (int) cast below. (plutosvg returns -1 for
	 * percentage dims without a container, 0 when neither width/height
	 * nor viewBox exists — both are "unresolvable" here.) */
	if (!isfinite(w) || !isfinite(h) || w <= 0 || h <= 0
	    || w >= (float)INT_MAX || h >= (float)INT_MAX) {
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

	/* Dims are only known post-parse, so the destination cannot be
	 * pre-allocated ahead of the document like fastchart_rasterize_svg
	 * does. Guard the allocation instead: a memory_limit bailout here
	 * would otherwise leak the malloc'd document persistently. */
	int rc;
	zend_try {
		pix->rgba = safe_emalloc((size_t)iw * (size_t)ih, 4, 0);
		rc = fastchart_rasterize_doc(doc, iw, ih, pix);
	} zend_catch {
		plutosvg_document_destroy(doc);
		if (pix->rgba) {
			efree(pix->rgba);
			pix->rgba = NULL;
		}
		zend_bailout();
	} zend_end_try();
	plutosvg_document_destroy(doc);
	if (rc != 0) {
		efree(pix->rgba);
		pix->rgba = NULL;
	}
	return (rc == 0) ? 0 : -4;
}
