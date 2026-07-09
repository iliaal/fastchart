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

  Raster encoders: RGBA pixel buffer -> PNG / JPEG / WebP, appended
  into a caller-owned smart_str.

  libpng:        the high-level API (png_set_compression_level default
                 6, RGBA streamed row by row).
  libjpeg-turbo: optimize_coding TRUE, 4:2:0 subsampling, non-progressive.
                 Matches the q88 reference established in the plutovg
                 quality-eval; RGB ingest, alpha is flattened over white
                 since JPEG has no alpha channel.
  libwebp:       WebPEncodeRGBA simple API for lossy at quality q;
                 alpha preserved.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fastchart_encoder.h"
#include "zend_smart_str.h"

#ifdef HAVE_LIBPNG
#include <png.h>
#endif
#ifdef HAVE_LIBJPEG
#include <jpeglib.h>
#include <jerror.h>
#endif
#ifdef HAVE_LIBWEBP
#include <webp/encode.h>
#endif

#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

/* Same SIMD gates as fastchart_rasterize.c: x86 uses SSSE3 runtime
 * dispatch via raw CPUID (see the rationale there — __builtin_cpu_supports
 * drags in libgcc's __cpu_model and breaks static/musl/zig dlopen);
 * AArch64 uses baseline NEON. */
#if (defined(__x86_64__) || defined(_M_X64)) && defined(__GNUC__)
#  define FC_ENC_HAVE_X86_SIMD 1
#  include <immintrin.h>
#  include <cpuid.h>
#endif
#if defined(__aarch64__) && (defined(__ARM_NEON) || defined(__ARM_NEON__))
#  define FC_ENC_HAVE_ARM_NEON 1
#  include <arm_neon.h>
#endif

#ifdef FC_ENC_HAVE_X86_SIMD
static int fc_enc_cpu_has_ssse3(void)
{
    static int cached = -1;
    if (cached < 0) {
        unsigned int eax, ebx, ecx, edx;
        cached = (__get_cpuid(1, &eax, &ebx, &ecx, &edx)
                  && (ecx & bit_SSSE3)) ? 1 : 0;
    }
    return cached;
}

/* Pack `n_pixels` opaque RGBA pixels at src into RGB at dst via SSSE3
 * _mm_shuffle_epi8 (4 pixels per instruction). Returns the count of
 * pixels processed (always a multiple of 4); the caller handles the
 * remainder with scalar copies. dst must have at least 16 bytes of
 * trailing slack so the 16-byte store on the final chunk can overrun
 * its 12 valid bytes without touching unowned memory. */
__attribute__((target("ssse3")))
static int fc_enc_pack_rgba_to_rgb_ssse3(const uint8_t *src, uint8_t *dst,
                                          int n_pixels)
{
    static const int8_t shuf_bytes[16] = {
         0,  1,  2,
         4,  5,  6,
         8,  9, 10,
        12, 13, 14,
        -1, -1, -1, -1  /* upper 4 lanes are don't-care */
    };
    __m128i shuf = _mm_loadu_si128((const __m128i *)shuf_bytes);
    int simd_end = n_pixels & ~3;
    for (int x = 0; x < simd_end; x += 4) {
        __m128i rgba = _mm_loadu_si128(
            (const __m128i *)(src + (size_t)x * 4));
        __m128i rgb = _mm_shuffle_epi8(rgba, shuf);
        _mm_storeu_si128((__m128i *)(dst + (size_t)x * 3), rgb);
    }
    return simd_end;
}
#endif

#ifdef FC_ENC_HAVE_ARM_NEON
/* Keep the NEON helper outside libjpeg's setjmp frame; AArch64 GCC can
 * otherwise warn about clobbered vector temporaries when it inlines this. */
__attribute__((noinline))
static int fc_enc_pack_rgba_to_rgb_neon(const uint8_t *src, uint8_t *dst,
                                         int n_pixels)
{
	int simd_end = n_pixels & ~15;  /* 16 RGBA pixels -> 48 RGB bytes */
	for (int x = 0; x < simd_end; x += 16) {
		uint8x16x4_t rgba = vld4q_u8(src + (size_t)x * 4);
		uint8x16x3_t rgb;
		rgb.val[0] = rgba.val[0];
		rgb.val[1] = rgba.val[1];
		rgb.val[2] = rgba.val[2];
		vst3q_u8(dst + (size_t)x * 3, rgb);
	}
	return simd_end;
}
#endif

/* Prewarm the SSSE3 capability cache and format the libwebp version
 * string at module load so neither lazy first-call path can race
 * under ZTS. */
#ifdef HAVE_LIBWEBP
static void fc_webp_version_init(void);
#endif

void fastchart_encoder_init(void)
{
#ifdef FC_ENC_HAVE_X86_SIMD
	(void)fc_enc_cpu_has_ssse3();
#endif
#ifdef HAVE_LIBWEBP
	fc_webp_version_init();
#endif
}

void fastchart_pixels_init(fastchart_pixels_t *pix, int w, int h)
{
	pix->rgba = NULL;
	pix->w    = w;
	pix->h    = h;
	pix->has_alpha = 0;
	pix->dpi  = 0;
	pix->png_level = -1;
}

void fastchart_pixels_release(fastchart_pixels_t *pix)
{
	if (pix->rgba) {
		efree(pix->rgba);
		pix->rgba = NULL;
	}
	pix->w = pix->h = 0;
}

/* --------------------------- PNG ----------------------------------- */

#ifdef HAVE_LIBPNG
static void png_smart_write(png_structp png, png_bytep data, png_size_t len)
{
	smart_str *out = (smart_str *)png_get_io_ptr(png);
	smart_str_appendl(out, (const char *)data, len);
}

static void png_smart_flush(png_structp png)
{
	(void)png;
}

static void png_smart_error(png_structp png, png_const_charp msg)
{
	(void)msg;
	longjmp(png_jmpbuf(png), 1);
}

int fastchart_encode_png(smart_str *out, const fastchart_pixels_t *pix)
{
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
	                                          NULL, png_smart_error, NULL);
	if (!png) return -1;
	png_infop info = png_create_info_struct(png);
	if (!info) {
		png_destroy_write_struct(&png, NULL);
		return -1;
	}

	/* png_smart_write appends into the caller's smart_str, so a
	 * memory_limit bailout can fire inside the live libpng session and
	 * longjmp past the destroy — leaking the write struct + zlib state
	 * (malloc'd, invisible to memory_limit) in a long-running worker.
	 * Release the vendor state before re-entering the bailout. */
	volatile int rc = 0;
	zend_try {

	if (setjmp(png_jmpbuf(png))) {
		png_destroy_write_struct(&png, &info);
		rc = -1;
		goto done;
	}

	png_set_write_fn(png, out, png_smart_write, png_smart_flush);

	/* Chart output is flat fills and anti-aliased edges, never
	 * photographic. libpng's default per-row adaptive filtering (try
	 * all five, keep the best) costs ~40% of the row-write stage here
	 * for a size difference under half a percent; a fixed UP filter
	 * keeps the size and drops the search. */
	png_set_filter(png, 0, PNG_FILTER_UP);
	if (pix->png_level >= 0) {
		png_set_compression_level(png, pix->png_level);
	}

	int color_type = pix->has_alpha ? PNG_COLOR_TYPE_RGB_ALPHA
	                                : PNG_COLOR_TYPE_RGB;
	png_set_IHDR(png, info, pix->w, pix->h, 8, color_type,
	             PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
	             PNG_FILTER_TYPE_DEFAULT);

	if (pix->dpi > 0) {
		/* DPI -> pixels per meter: 1 inch = 0.0254 m. Mirrors what
		 * libgd's gdImageSetResolution writes into pHYs. */
		png_uint_32 ppm = (png_uint_32)((double)pix->dpi / 0.0254 + 0.5);
		png_set_pHYs(png, info, ppm, ppm, PNG_RESOLUTION_METER);
	}

	png_write_info(png, info);

	if (!pix->has_alpha) {
		/* Strip the A byte per pixel during write. libpng can pack
		 * from RGBA if we tell it to filter the trailing channel. */
		png_set_filler(png, 0, PNG_FILLER_AFTER);
	}

	const uint8_t *row = pix->rgba;
	for (int y = 0; y < pix->h; y++) {
		png_write_row(png, (png_bytep)row);
		row += pix->w * 4;
	}
	png_write_end(png, info);

	png_destroy_write_struct(&png, &info);
done:;

	} zend_catch {
		png_destroy_write_struct(&png, &info);
		zend_bailout();
	} zend_end_try();

	return rc;
}

int fastchart_have_libpng(void)         { return 1; }
const char *fastchart_libpng_version(void) { return PNG_LIBPNG_VER_STRING; }
#else  /* !HAVE_LIBPNG */
int fastchart_encode_png(smart_str *out, const fastchart_pixels_t *pix)
{
	(void)out; (void)pix;
	return -2;
}
int fastchart_have_libpng(void)         { return 0; }
const char *fastchart_libpng_version(void) { return NULL; }
#endif

/* --------------------------- JPEG ---------------------------------- */

#ifdef HAVE_LIBJPEG
struct fc_jpeg_err {
	struct jpeg_error_mgr base;
	jmp_buf jmp;
};

static void fc_jpeg_error_exit(j_common_ptr cinfo)
{
	struct fc_jpeg_err *e = (struct fc_jpeg_err *)cinfo->err;
	longjmp(e->jmp, 1);
}

/* Custom destination manager appending straight into the caller's
 * smart_str, mirroring png_smart_write. jpeg_mem_dest is avoided on
 * purpose: it publishes the final buffer address only at
 * term_destination, so once its internal buffer grows past the initial
 * allocation the caller-side pointer dangles at a block
 * empty_mem_output_buffer already freed — an error_exit longjmp
 * mid-encode would then double-free it and leak the live grown buffer.
 * Streaming into the smart_str leaves no malloc'd intermediate to
 * clean up on either path. */
#define FC_JPEG_STAGE_SZ 8192

struct fc_jpeg_dest {
	struct jpeg_destination_mgr base;
	smart_str *out;
	JOCTET stage[FC_JPEG_STAGE_SZ];
};

static void fc_jpeg_init_destination(j_compress_ptr cinfo)
{
	struct fc_jpeg_dest *d = (struct fc_jpeg_dest *)cinfo->dest;
	d->base.next_output_byte = d->stage;
	d->base.free_in_buffer   = FC_JPEG_STAGE_SZ;
}

static boolean fc_jpeg_empty_output_buffer(j_compress_ptr cinfo)
{
	struct fc_jpeg_dest *d = (struct fc_jpeg_dest *)cinfo->dest;
	/* libjpeg's contract: flush the WHOLE stage buffer here regardless
	 * of free_in_buffer (jdatadst.c documents the same). */
	smart_str_appendl(d->out, (const char *)d->stage, FC_JPEG_STAGE_SZ);
	d->base.next_output_byte = d->stage;
	d->base.free_in_buffer   = FC_JPEG_STAGE_SZ;
	return TRUE;
}

static void fc_jpeg_term_destination(j_compress_ptr cinfo)
{
	struct fc_jpeg_dest *d = (struct fc_jpeg_dest *)cinfo->dest;
	size_t used = FC_JPEG_STAGE_SZ - d->base.free_in_buffer;
	if (used > 0) {
		smart_str_appendl(d->out, (const char *)d->stage, used);
	}
}

int fastchart_encode_jpeg(smart_str *out, const fastchart_pixels_t *pix,
                          int quality, int bg_rgb)
{
	if (quality < 1)   quality = 1;
	if (quality > 100) quality = 100;

	struct jpeg_compress_struct cinfo;
	struct fc_jpeg_err err;
	cinfo.err = jpeg_std_error(&err.base);
	err.base.error_exit = fc_jpeg_error_exit;

	struct fc_jpeg_dest dest;
	dest.out = out;

	/* `volatile` keeps the pointer's live value in memory across the
	 * setjmp boundary. Per C99 §7.13.2.1, automatic-storage locals
	 * modified after setjmp() and read in the longjmp() recovery branch
	 * have indeterminate values unless declared volatile. Without it,
	 * the optimizer may keep rgb_row in a register that libjpeg's
	 * longjmp() clobbers — the cleanup branch would then read a stale
	 * NULL or garbage and either leak the allocation or double-free. */
	uint8_t * volatile rgb_row = NULL;
	/* Gate the destroy on a completed create: if jpeg_create_compress
	 * itself longjmps (struct/version mismatch), cinfo.mem is still
	 * uninitialized and jpeg_destroy_compress must not run on it. */
	volatile int created = 0;
	volatile int rc = 0;

	/* The destination callbacks append into the caller's smart_str, so
	 * a memory_limit bailout can fire mid-session and longjmp past
	 * jpeg_destroy_compress — with optimize_coding libjpeg is holding
	 * whole-image coefficient buffers (malloc'd, invisible to
	 * memory_limit). Release them before re-entering the bailout. */
	zend_try {

	if (setjmp(err.jmp)) {
		if (created) jpeg_destroy_compress(&cinfo);
		if (rgb_row)  efree(rgb_row);
		/* Partial bytes already streamed into `out` are the caller's
		 * to discard (smart_str_free on the error branch) — same
		 * convention as the PNG encoder. */
		rc = -1;
		goto done;
	}

	jpeg_create_compress(&cinfo);
	created = 1;
	dest.base.init_destination    = fc_jpeg_init_destination;
	dest.base.empty_output_buffer = fc_jpeg_empty_output_buffer;
	dest.base.term_destination    = fc_jpeg_term_destination;
	cinfo.dest = &dest.base;

	cinfo.image_width      = pix->w;
	cinfo.image_height     = pix->h;
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);
	cinfo.optimize_coding = TRUE;
	if (pix->dpi > 0) {
		cinfo.density_unit = 1;            /* dots per inch */
		cinfo.X_density = (UINT16)pix->dpi;
		cinfo.Y_density = (UINT16)pix->dpi;
	}
	/* 4:2:0 chroma subsampling — matches the eval reference. Setting
	 * it explicitly because jpeg_set_quality flips to 4:4:4 above
	 * q=90 in some libjpeg-turbo versions. */
	cinfo.comp_info[0].h_samp_factor = 2;
	cinfo.comp_info[0].v_samp_factor = 2;
	cinfo.comp_info[1].h_samp_factor = 1;
	cinfo.comp_info[1].v_samp_factor = 1;
	cinfo.comp_info[2].h_samp_factor = 1;
	cinfo.comp_info[2].v_samp_factor = 1;

	jpeg_start_compress(&cinfo, TRUE);

	/* Per-row RGBA -> RGB strip. Two paths:
	 *   - opaque (pix->has_alpha == 0, set by fastchart_rasterize_doc's
	 *     opaque-detect): SSSE3 on x86_64 or NEON on AArch64 packs
	 *     pixels in vector chunks; scalar straight copy handles the rest.
	 *   - translucent: scalar alpha-flatten over bg. JPEG has no
	 *     alpha channel, so transparent regions show through as the
	 *     composited background. */
	/* Computed past the setjmp() above so it isn't flagged -Wclobbered;
	 * set once here and only read in the loop. Negative bg_rgb (the
	 * chart-side default) keeps the historical white fill. */
	uint8_t bg_r = 255, bg_g = 255, bg_b = 255;
	if (bg_rgb >= 0) {
		bg_r = (uint8_t)((bg_rgb >> 16) & 0xFF);
		bg_g = (uint8_t)((bg_rgb >>  8) & 0xFF);
		bg_b = (uint8_t)( bg_rgb        & 0xFF);
	}
	rgb_row = emalloc((size_t)pix->w * 3 + 16);  /* +16 trailing slack for x86 SSE store overrun */
	for (int y = 0; y < pix->h; y++) {
		const uint8_t *src = pix->rgba + (size_t)y * pix->w * 4;
		uint8_t       *dst = rgb_row;
		int x = 0;
		if (!pix->has_alpha) {
#ifdef FC_ENC_HAVE_ARM_NEON
			x = fc_enc_pack_rgba_to_rgb_neon(src, dst, pix->w);
#elif defined(FC_ENC_HAVE_X86_SIMD)
			if (fc_enc_cpu_has_ssse3()) {
				x = fc_enc_pack_rgba_to_rgb_ssse3(src, dst, pix->w);
			}
#endif
			for (; x < pix->w; x++) {
				dst[(size_t)x * 3 + 0] = src[(size_t)x * 4 + 0];
				dst[(size_t)x * 3 + 1] = src[(size_t)x * 4 + 1];
				dst[(size_t)x * 3 + 2] = src[(size_t)x * 4 + 2];
			}
		} else {
			for (x = 0; x < pix->w; x++) {
				const uint8_t *p = src + (size_t)x * 4;
				uint8_t       *q = dst + (size_t)x * 3;
				uint8_t r = p[0], g = p[1], b = p[2], a = p[3];
				if (a == 255) {
					q[0] = r; q[1] = g; q[2] = b;
				} else if (a == 0) {
					q[0] = bg_r; q[1] = bg_g; q[2] = bg_b;
				} else {
					/* over bg: out = src*a + bg*(255-a)/255 */
					int ia = 255 - a;
					q[0] = (uint8_t)((r * a + bg_r * ia) / 255);
					q[1] = (uint8_t)((g * a + bg_g * ia) / 255);
					q[2] = (uint8_t)((b * a + bg_b * ia) / 255);
				}
			}
		}
		JSAMPROW row = rgb_row;
		jpeg_write_scanlines(&cinfo, &row, 1);
	}
	efree(rgb_row);
	rgb_row = NULL;

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
done:;

	} zend_catch {
		if (created) jpeg_destroy_compress(&cinfo);
		if (rgb_row) efree(rgb_row);
		zend_bailout();
	} zend_end_try();

	return rc;
}

int fastchart_have_libjpeg(void) { return 1; }
/* Value reported in the MINFO row labelled "libjpeg". LIBJPEG_TURBO_-
 * VERSION expands to bare 2.1.2 in jconfig.h (not a string literal),
 * so stringify it. The "(turbo)" suffix distinguishes from a
 * reference-libjpeg build where only the JPEG_LIB_VERSION API number
 * is available. */
#define FC_STR(x) #x
#define FC_XSTR(x) FC_STR(x)
const char *fastchart_libjpeg_version(void)
{
#ifdef LIBJPEG_TURBO_VERSION
	return FC_XSTR(LIBJPEG_TURBO_VERSION) " (turbo)";
#else
	return "API " FC_XSTR(JPEG_LIB_VERSION);
#endif
}
#else  /* !HAVE_LIBJPEG */
int fastchart_encode_jpeg(smart_str *out, const fastchart_pixels_t *pix,
                          int quality, int bg_rgb)
{
	(void)out; (void)pix; (void)quality; (void)bg_rgb;
	return -2;
}
int fastchart_have_libjpeg(void)         { return 0; }
const char *fastchart_libjpeg_version(void) { return NULL; }
#endif

/* --------------------------- WebP ---------------------------------- */

#ifdef HAVE_LIBWEBP
/* Advanced API instead of WebPEncodeRGBA/WebPEncodeRGB. Three knobs
 * matter for chart-shaped content (flat fills, sharp edges, small
 * palette):
 *   - WEBP_PRESET_DRAWING tunes the entropy analysis for line-art /
 *     drawing content. WEBP_PRESET_DEFAULT (the simple API's choice)
 *     spends analysis cycles on photographic detail charts don't have.
 *   - method = 2 trades a few percent of file size for a 2-3x encode
 *     speedup. The simple API defaults to method = 4 (moderate).
 *   - thread_level = 1 enables parallel entropy encoding on libwebp
 *     builds compiled with pthread support; the simple API never
 *     opts in.
 *
 * The encoder still produces a baseline-compatible .webp stream that
 * every conformant decoder accepts. */
int fastchart_encode_webp(smart_str *out, const fastchart_pixels_t *pix,
                          int quality, int mode)
{
	float q = (float)quality;
	if (q < 1.0f)   q = 1.0f;
	if (q > 100.0f) q = 100.0f;

	WebPConfig config;
	switch (mode) {
	case FASTCHART_WEBP_PHOTO:
		if (!WebPConfigPreset(&config, WEBP_PRESET_PHOTO, q)) return -1;
		config.method = 4;
		break;
	case FASTCHART_WEBP_LOSSLESS:
		/* Lossless ignores perceptual quality; the encoder picks
		 * compression strategy from method. method=6 is the
		 * highest-compression setting and is the right pick when
		 * the user asks for "lossless" (size matters more than
		 * speed in that scenario). */
		if (!WebPConfigPreset(&config, WEBP_PRESET_DEFAULT, q)) return -1;
		config.lossless = 1;
		config.method = 6;
		break;
	case FASTCHART_WEBP_FAST:
		if (!WebPConfigPreset(&config, WEBP_PRESET_DRAWING, q)) return -1;
		config.method = 0;
		break;
	case FASTCHART_WEBP_DRAWING:
	default:
		if (!WebPConfigPreset(&config, WEBP_PRESET_DRAWING, q)) return -1;
		config.method = 2;
		break;
	}
	config.thread_level = 1;

	WebPPicture picture;
	if (!WebPPictureInit(&picture)) {
		return -1;
	}
	picture.width  = pix->w;
	picture.height = pix->h;
	/* Lossless must import straight into the ARGB plane: with
	 * use_argb == 0 libwebp converts RGBA to YUV420 at import time
	 * (4:2:0 chroma decimation — lossy per webp/encode.h), and VP8L
	 * would encode the degraded pixels. Lossy modes keep the YUV
	 * import; it is their native fast path and the opaque-detect
	 * note below depends on it. */
	picture.use_argb = (mode == FASTCHART_WEBP_LOSSLESS);

	/* Always import RGBA — no manual RGB pack. When the input is
	 * opaque (pix->has_alpha == 0, set by fastchart_rasterize_doc's
	 * opaque-detect), libwebp's internal WebPPictureHasTransparency
	 * sees all-FF alphas and skips the alpha plane during YUV
	 * conversion. Saves a w*h*3 emalloc and a per-pixel scalar
	 * copy that previously dominated the encoder's CPU on opaque
	 * charts. */
	if (!WebPPictureImportRGBA(&picture, pix->rgba, pix->w * 4)) {
		WebPPictureFree(&picture);
		return -1;
	}

	WebPMemoryWriter writer;
	WebPMemoryWriterInit(&writer);
	picture.writer     = WebPMemoryWrite;
	picture.custom_ptr = &writer;

	int enc_ok = WebPEncode(&config, &picture);
	WebPPictureFree(&picture);

	if (!enc_ok || writer.size == 0) {
		WebPMemoryWriterClear(&writer);
		return -1;
	}
	/* The append can bail on memory_limit with the malloc'd encoded
	 * file (writer.mem) live — release it before re-entering the
	 * bailout. */
	zend_try {
		smart_str_appendl(out, (const char *)writer.mem, writer.size);
	} zend_catch {
		WebPMemoryWriterClear(&writer);
		zend_bailout();
	} zend_end_try();
	WebPMemoryWriterClear(&writer);
	return 0;
}

int fastchart_have_libwebp(void) { return 1; }

/* Formatted once at MINIT (fastchart_encoder_init): a per-call
 * snprintf into the shared static was a benign-but-racy write under
 * ZTS when two threads hit MINFO concurrently. */
static char fc_webp_ver[16];

static void fc_webp_version_init(void)
{
	int v = WebPGetEncoderVersion();
	snprintf(fc_webp_ver, sizeof(fc_webp_ver), "%d.%d.%d",
	         (v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

const char *fastchart_libwebp_version(void)
{
	return fc_webp_ver;
}
#else  /* !HAVE_LIBWEBP */
int fastchart_encode_webp(smart_str *out, const fastchart_pixels_t *pix,
                          int quality, int mode)
{
	(void)out; (void)pix; (void)quality; (void)mode;
	return -2;
}
int fastchart_have_libwebp(void)         { return 0; }
const char *fastchart_libwebp_version(void) { return NULL; }
#endif
