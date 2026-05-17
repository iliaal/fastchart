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

void fastchart_pixels_init(fastchart_pixels_t *pix, int w, int h)
{
	pix->rgba = NULL;
	pix->w    = w;
	pix->h    = h;
	pix->has_alpha = 0;
	pix->dpi  = 0;
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
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_write_struct(&png, &info);
		return -1;
	}

	png_set_write_fn(png, out, png_smart_write, png_smart_flush);

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
	return 0;
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

/* libjpeg's memory destination manager (writes to an in-process
 * malloc buffer). We then copy into the smart_str. The library
 * provides jpeg_mem_dest in modern libjpeg-turbo. */

int fastchart_encode_jpeg(smart_str *out, const fastchart_pixels_t *pix,
                          int quality)
{
	if (quality < 1)   quality = 1;
	if (quality > 100) quality = 100;

	struct jpeg_compress_struct cinfo;
	struct fc_jpeg_err err;
	cinfo.err = jpeg_std_error(&err.base);
	err.base.error_exit = fc_jpeg_error_exit;

	/* `volatile` keeps the pointer's live value in memory across the
	 * setjmp boundary. Per C99 §7.13.2.1, automatic-storage locals
	 * modified after setjmp() and read in the longjmp() recovery branch
	 * have indeterminate values unless declared volatile. Without it,
	 * the optimizer may keep jpeg_buf / rgb_row in a register that
	 * libjpeg's longjmp() clobbers — the cleanup branch would then read
	 * a stale NULL or garbage and either leak the allocation or
	 * double-free. */
	unsigned char * volatile jpeg_buf = NULL;
	unsigned long            jpeg_sz  = 0;
	uint8_t       * volatile rgb_row  = NULL;

	if (setjmp(err.jmp)) {
		jpeg_destroy_compress(&cinfo);
		if (jpeg_buf) free(jpeg_buf);
		if (rgb_row)  efree(rgb_row);
		return -1;
	}

	jpeg_create_compress(&cinfo);
	/* The cast strips the `volatile` qualifier on jpeg_buf for the
	 * libjpeg API (which takes a plain `unsigned char **`). volatile
	 * here governs how the compiler optimizes our own access across
	 * setjmp/longjmp; it doesn't change the underlying storage. */
	jpeg_mem_dest(&cinfo, (unsigned char **)(uintptr_t)&jpeg_buf, &jpeg_sz);

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

	/* Per-row RGBA → RGB strip; alpha is flattened over white because
	 * JPEG has no alpha channel. Most fastchart raster output is
	 * already opaque, but we handle the alpha path defensively. */
	rgb_row = emalloc((size_t)pix->w * 3);
	for (int y = 0; y < pix->h; y++) {
		const uint8_t *src = pix->rgba + (size_t)y * pix->w * 4;
		uint8_t       *dst = rgb_row;
		for (int x = 0; x < pix->w; x++) {
			uint8_t r = src[0], g = src[1], b = src[2], a = src[3];
			if (a == 255) {
				dst[0] = r; dst[1] = g; dst[2] = b;
			} else if (a == 0) {
				dst[0] = 255; dst[1] = 255; dst[2] = 255;
			} else {
				/* over white: out = src*a + 255*(255-a)/255 */
				int ia = 255 - a;
				dst[0] = (uint8_t)((r * a + 255 * ia) / 255);
				dst[1] = (uint8_t)((g * a + 255 * ia) / 255);
				dst[2] = (uint8_t)((b * a + 255 * ia) / 255);
			}
			src += 4;
			dst += 3;
		}
		JSAMPROW r = rgb_row;
		jpeg_write_scanlines(&cinfo, &r, 1);
	}
	efree(rgb_row);
	rgb_row = NULL;

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	smart_str_appendl(out, (const char *)jpeg_buf, (size_t)jpeg_sz);
	free(jpeg_buf);
	return 0;
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
                          int quality)
{
	(void)out; (void)pix; (void)quality;
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
	picture.use_argb = 0;

	int import_ok;
	if (pix->has_alpha) {
		import_ok = WebPPictureImportRGBA(
			&picture, pix->rgba, pix->w * 4);
	} else {
		/* Pack down to RGB so the encoder doesn't carry an alpha
		 * channel through entropy coding on opaque output. */
		uint8_t *rgb = emalloc((size_t)pix->w * pix->h * 3);
		const uint8_t *src = pix->rgba;
		uint8_t       *dst = rgb;
		size_t pixels = (size_t)pix->w * pix->h;
		for (size_t i = 0; i < pixels; i++) {
			dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
			src += 4; dst += 3;
		}
		import_ok = WebPPictureImportRGB(&picture, rgb, pix->w * 3);
		efree(rgb);
	}
	if (!import_ok) {
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
	smart_str_appendl(out, (const char *)writer.mem, writer.size);
	WebPMemoryWriterClear(&writer);
	return 0;
}

int fastchart_have_libwebp(void) { return 1; }
const char *fastchart_libwebp_version(void)
{
	static char ver[16];
	int v = WebPGetEncoderVersion();
	snprintf(ver, sizeof(ver), "%d.%d.%d",
	         (v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
	return ver;
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
