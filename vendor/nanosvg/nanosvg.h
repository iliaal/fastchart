/*
 * Copyright (c) 2013-14 Mikko Mononen memon@inside.org
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*
 * fastchart-local trim: this is the upstream NanoSVG header reduced to
 * just the public type declarations the rasterizer needs (NSVGpaint,
 * NSVGgradient, NSVGpath, NSVGshape, NSVGimage and the supporting
 * enums). Upstream NanoSVG also ships a 3000-line SVG-string parser
 * implementation here; fastchart constructs path data programmatically
 * from gdPoint arrays and never calls the parser, so the parser code
 * and its parser-only function declarations are not vendored. See
 * vendor/nanosvg/VENDOR.md for the diff against upstream.
 */

#ifndef NANOSVG_H
#define NANOSVG_H

#ifndef NANOSVG_CPLUSPLUS
#ifdef __cplusplus
extern "C" {
#endif
#endif

enum NSVGpaintType {
	NSVG_PAINT_UNDEF = -1,
	NSVG_PAINT_NONE = 0,
	NSVG_PAINT_COLOR = 1,
	NSVG_PAINT_LINEAR_GRADIENT = 2,
	NSVG_PAINT_RADIAL_GRADIENT = 3
};

enum NSVGspreadType {
	NSVG_SPREAD_PAD = 0,
	NSVG_SPREAD_REFLECT = 1,
	NSVG_SPREAD_REPEAT = 2
};

enum NSVGlineJoin {
	NSVG_JOIN_MITER = 0,
	NSVG_JOIN_ROUND = 1,
	NSVG_JOIN_BEVEL = 2
};

enum NSVGlineCap {
	NSVG_CAP_BUTT = 0,
	NSVG_CAP_ROUND = 1,
	NSVG_CAP_SQUARE = 2
};

enum NSVGfillRule {
	NSVG_FILLRULE_NONZERO = 0,
	NSVG_FILLRULE_EVENODD = 1
};

enum NSVGflags {
	NSVG_FLAGS_VISIBLE = 0x01
};

enum NSVGpaintOrder {
	NSVG_PAINT_FILL = 0x00,
	NSVG_PAINT_MARKERS = 0x01,
	NSVG_PAINT_STROKE = 0x02,
};

typedef struct NSVGgradientStop {
	unsigned int color;
	float offset;
} NSVGgradientStop;

typedef struct NSVGgradient {
	float xform[6];
	char spread;
	float fx, fy;
	int nstops;
	NSVGgradientStop stops[1];
} NSVGgradient;

typedef struct NSVGpaint {
	signed char type;
	union {
		unsigned int color;
		NSVGgradient* gradient;
	};
} NSVGpaint;

typedef struct NSVGpath
{
	float* pts;					/* Cubic bezier points: x0,y0, [cpx1,cpx1,cpx2,cpy2,x1,y1], ... */
	int npts;					/* Total number of bezier points. */
	char closed;				/* Flag indicating if shapes should be treated as closed. */
	float bounds[4];			/* Tight bounding box of the shape [minx,miny,maxx,maxy]. */
	struct NSVGpath* next;		/* Pointer to next path, or NULL if last element. */
} NSVGpath;

typedef struct NSVGshape
{
	char id[64];				/* Optional 'id' attr of the shape or its group */
	NSVGpaint fill;				/* Fill paint */
	NSVGpaint stroke;			/* Stroke paint */
	float opacity;				/* Opacity of the shape. */
	float strokeWidth;			/* Stroke width (scaled). */
	float strokeDashOffset;		/* Stroke dash offset (scaled). */
	float strokeDashArray[8];	/* Stroke dash array (scaled). */
	char strokeDashCount;		/* Number of dash values in dash array. */
	char strokeLineJoin;		/* Stroke join type. */
	char strokeLineCap;			/* Stroke cap type. */
	float miterLimit;			/* Miter limit */
	char fillRule;				/* Fill rule, see NSVGfillRule. */
	unsigned char paintOrder;	/* Encoded paint order (3x2-bit fields), see NSVGpaintOrder */
	unsigned char flags;		/* Logical or of NSVG_FLAGS_* flags */
	float bounds[4];			/* Tight bounding box of the shape [minx,miny,maxx,maxy]. */
	char fillGradient[64];		/* Optional 'id' of fill gradient */
	char strokeGradient[64];	/* Optional 'id' of stroke gradient */
	float xform[6];				/* Root transformation for fill/stroke gradient */
	NSVGpath* paths;			/* Linked list of paths in the image. */
	struct NSVGshape* next;		/* Pointer to next shape, or NULL if last element. */
} NSVGshape;

typedef struct NSVGimage
{
	float width;				/* Width of the image. */
	float height;				/* Height of the image. */
	NSVGshape* shapes;			/* Linked list of shapes in the image. */
} NSVGimage;

#ifndef NANOSVG_CPLUSPLUS
#ifdef __cplusplus
}
#endif
#endif

#endif /* NANOSVG_H */
