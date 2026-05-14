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

#include "fastchart_encoder.h"
#include "zend_smart_str.h"

#include <png.h>
#include <jpeglib.h>
#include <jerror.h>
#include <webp/encode.h>

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

/* --------------------------- JPEG ---------------------------------- */

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

	unsigned char *jpeg_buf = NULL;
	unsigned long  jpeg_sz  = 0;
	uint8_t       *rgb_row  = NULL;

	if (setjmp(err.jmp)) {
		jpeg_destroy_compress(&cinfo);
		if (jpeg_buf) free(jpeg_buf);
		if (rgb_row)  efree(rgb_row);
		return -1;
	}

	jpeg_create_compress(&cinfo);
	jpeg_mem_dest(&cinfo, &jpeg_buf, &jpeg_sz);

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

/* --------------------------- WebP ---------------------------------- */

int fastchart_encode_webp(smart_str *out, const fastchart_pixels_t *pix,
                          int quality)
{
	float q = (float)quality;
	if (q < 1.0f)   q = 1.0f;
	if (q > 100.0f) q = 100.0f;

	uint8_t *buf = NULL;
	size_t   sz;
	if (pix->has_alpha) {
		sz = WebPEncodeRGBA(pix->rgba, pix->w, pix->h, pix->w * 4, q, &buf);
	} else {
		/* libwebp's RGB encoder wants RGB (3 bytes/pixel). Strip alpha. */
		uint8_t *rgb = emalloc((size_t)pix->w * pix->h * 3);
		const uint8_t *src = pix->rgba;
		uint8_t       *dst = rgb;
		size_t pixels = (size_t)pix->w * pix->h;
		for (size_t i = 0; i < pixels; i++) {
			dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
			src += 4; dst += 3;
		}
		sz = WebPEncodeRGB(rgb, pix->w, pix->h, pix->w * 3, q, &buf);
		efree(rgb);
	}
	if (sz == 0 || buf == NULL) {
		if (buf) WebPFree(buf);
		return -1;
	}
	smart_str_appendl(out, (const char *)buf, sz);
	WebPFree(buf);
	return 0;
}
