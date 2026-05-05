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
*/

/* Bridge: NanoSVG software rasterizer for shapes that libgd renders
 * with hard-edge fills. We hand-build NSVGimage/NSVGshape/NSVGpath
 * structures from gdPoint arrays, ask the rasterizer for a tight
 * RGBA32 buffer over the polygon's bounding box, then alpha-composite
 * that buffer onto the destination gdImagePtr.
 *
 * Why this exists: libgd's gdImageFilledPolygon paints with binary
 * coverage (each pixel is fully inside or fully outside). NanoSVG's
 * rasterizer computes fractional coverage per pixel and emits proper
 * alpha-blended edge pixels — what every modern 2D library
 * (Cairo, Skia, browser canvas) does by default. Visible win on
 * angled-edge shapes (pie slices, gauge wedges, area / radar / polar
 * fills).
 *
 * What this is NOT: a general-purpose drawing API. It only handles
 * polygon fill (closed path) and arc/wedge fill. Stroke goes through
 * libgd's existing gdImageLine / SetAntiAliased path; we don't need
 * NanoSVG's stroke rasterizer because thin AA lines already look
 * fine through libgd's gdAntiAliased mode. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>

#include "php.h"
#include "fastchart_aa.h"

#include <gd.h>

#define NANOSVGRAST_IMPLEMENTATION
#include "vendor/nanosvg/nanosvgrast.h"

/* One rasterizer per render. Cheap to allocate (small struct) but
 * carries scratch state so a single static instance per draw avoids
 * thrashing. The caller manages lifetime via fastchart_aa_open /
 * fastchart_aa_close so a render that never paints AA polygons pays
 * nothing. */
struct fastchart_aa_ctx {
    NSVGrasterizer *rast;
};

fastchart_aa_ctx *fastchart_aa_open(void)
{
    fastchart_aa_ctx *ctx = emalloc(sizeof(*ctx));
    ctx->rast = nsvgCreateRasterizer();
    if (!ctx->rast) {
        efree(ctx);
        return NULL;
    }
    return ctx;
}

void fastchart_aa_close(fastchart_aa_ctx *ctx)
{
    if (!ctx) return;
    if (ctx->rast) nsvgDeleteRasterizer(ctx->rast);
    efree(ctx);
}

/* Composite an RGBA32 source buffer onto the gdImage at (dst_x, dst_y).
 * The source is row-major, stride bytes per row. NanoSVG outputs
 * straight (non-premultiplied) RGBA where alpha is in the high byte:
 * the blend is the standard "src OVER dst" formula:
 *
 *   out_rgb = src_rgb * src_a + dst_rgb * (1 - src_a)
 *   out_a   = 1   (we always write to opaque truecolor)
 *
 * We read the existing dst pixel, blend, and write back. This is hot —
 * a 200x200 polygon hits 40K pixels — so we go through the gd raw
 * pixel pointer (im->tpixels[y][x]) directly instead of the
 * gdImageGetPixel/SetPixel function pair. The truecolor-canvas guard
 * at draw entry guarantees tpixels is non-null. */
static void composite_rgba_onto_gd(gdImagePtr im, const unsigned char *src,
                                   int src_w, int src_h, int stride,
                                   int dst_x, int dst_y)
{
    int W = gdImageSX(im);
    int H = gdImageSY(im);
    for (int y = 0; y < src_h; y++) {
        int dy = dst_y + y;
        if (dy < 0 || dy >= H) continue;
        const unsigned char *row = src + (size_t)y * stride;
        int *dst_row = im->tpixels[dy];
        for (int x = 0; x < src_w; x++) {
            int dx = dst_x + x;
            if (dx < 0 || dx >= W) continue;
            unsigned int a = row[x * 4 + 3];
            if (a == 0) continue;  /* fully transparent — skip */
            unsigned int sr = row[x * 4 + 0];
            unsigned int sg = row[x * 4 + 1];
            unsigned int sb = row[x * 4 + 2];
            if (a == 255) {
                /* fully opaque — short-circuit the blend */
                dst_row[dx] = (sr << 16) | (sg << 8) | sb;
                continue;
            }
            int existing = dst_row[dx];
            unsigned int dr = (existing >> 16) & 0xFF;
            unsigned int dg = (existing >>  8) & 0xFF;
            unsigned int db =  existing        & 0xFF;
            unsigned int ia = 255 - a;
            unsigned int orr = (sr * a + dr * ia + 127) / 255;
            unsigned int og  = (sg * a + dg * ia + 127) / 255;
            unsigned int ob  = (sb * a + db * ia + 127) / 255;
            dst_row[dx] = (orr << 16) | (og << 8) | ob;
        }
    }
}

/* Build an NSVGpath from a gdPoint polygon. NanoSVG paths are stored
 * as cubic Bezier control points: [x0,y0, cp1x,cp1y, cp2x,cp2y, x1,y1, ...]
 * We emit a degenerate cubic for each polygon edge — both control
 * points coincide with the endpoints — which renders as a straight
 * line at no quality cost. The pts buffer is owned by the caller.
 *
 * Closed polygon: NanoSVG signals closure via the `closed` flag
 * (no need to append the start point at the end). */
static void build_polygon_path(NSVGpath *path, const gdPoint *poly, int n,
                               float *pts_buf)
{
    /* Each edge is one cubic = 6 points (cp1, cp2, end). The first
     * MoveTo emits 2 points (x0, y0). Total = 2 + 6 * n_edges, where
     * n_edges = n (closed) or n-1 (open). We always close. */
    int idx = 0;
    pts_buf[idx++] = (float)poly[0].x;
    pts_buf[idx++] = (float)poly[0].y;
    for (int i = 1; i < n; i++) {
        float ax = (float)poly[i - 1].x;
        float ay = (float)poly[i - 1].y;
        float bx = (float)poly[i].x;
        float by = (float)poly[i].y;
        pts_buf[idx++] = ax + (bx - ax) / 3.0f;
        pts_buf[idx++] = ay + (by - ay) / 3.0f;
        pts_buf[idx++] = ax + (bx - ax) * 2.0f / 3.0f;
        pts_buf[idx++] = ay + (by - ay) * 2.0f / 3.0f;
        pts_buf[idx++] = bx;
        pts_buf[idx++] = by;
    }
    /* Close the loop: cubic from last vertex back to first. */
    {
        float ax = (float)poly[n - 1].x;
        float ay = (float)poly[n - 1].y;
        float bx = (float)poly[0].x;
        float by = (float)poly[0].y;
        pts_buf[idx++] = ax + (bx - ax) / 3.0f;
        pts_buf[idx++] = ay + (by - ay) / 3.0f;
        pts_buf[idx++] = ax + (bx - ax) * 2.0f / 3.0f;
        pts_buf[idx++] = ay + (by - ay) * 2.0f / 3.0f;
        pts_buf[idx++] = bx;
        pts_buf[idx++] = by;
    }

    path->pts = pts_buf;
    path->npts = idx / 2;
    path->closed = 1;
    /* Bounds get computed by the rasterizer; setting them explicitly
     * isn't required for correctness, but populating them lets
     * NanoSVG short-circuit empty / out-of-clip paths cheaply. */
    float minx = pts_buf[0], miny = pts_buf[1];
    float maxx = pts_buf[0], maxy = pts_buf[1];
    for (int i = 2; i < idx; i += 2) {
        if (pts_buf[i] < minx) minx = pts_buf[i];
        if (pts_buf[i] > maxx) maxx = pts_buf[i];
        if (pts_buf[i + 1] < miny) miny = pts_buf[i + 1];
        if (pts_buf[i + 1] > maxy) maxy = pts_buf[i + 1];
    }
    path->bounds[0] = minx;
    path->bounds[1] = miny;
    path->bounds[2] = maxx;
    path->bounds[3] = maxy;
    path->next = NULL;
}

void fastchart_aa_filled_polygon(fastchart_aa_ctx *ctx, gdImagePtr im,
                                 const gdPoint *poly, int n_pts, int color)
{
    if (!ctx || !ctx->rast || n_pts < 3) return;
    /* libgd handle -> straight RGBA. Truecolor color packing on a
     * truecolor canvas is the int directly: 0xAARRGGBB. The canvas is
     * truecolor by virtue of the require_truecolor guard at draw
     * entry; we treat alpha as 0 (fully opaque) here unless the
     * caller passed an alpha-blended color (libgd encodes alpha 0-127
     * in the upper byte where 0 = opaque, 127 = transparent). */
    int gd_alpha = (color >> 24) & 0x7F;
    int sr = (color >> 16) & 0xFF;
    int sg = (color >>  8) & 0xFF;
    int sb =  color        & 0xFF;
    /* NanoSVG paint uses 0xAABBGGRR (alpha-low-then-blue-green-red) */
    unsigned int a8 = (unsigned int)(255 - (gd_alpha * 255 / 127));
    unsigned int nsvg_color = (a8 << 24) | ((unsigned)sb << 16)
                            | ((unsigned)sg << 8)  |  (unsigned)sr;

    /* Allocate the path's points buffer inline. Edges = n_pts (closed),
     * each edge is one cubic = 3 points after the initial MoveTo's 2,
     * so total floats = 2 + 6 * n_pts. */
    int n_floats = 2 + 6 * n_pts;
    float *pts_buf = emalloc((size_t)n_floats * sizeof(float));

    NSVGpath path;
    memset(&path, 0, sizeof(path));
    build_polygon_path(&path, poly, n_pts, pts_buf);

    NSVGshape shape;
    memset(&shape, 0, sizeof(shape));
    shape.fill.type = NSVG_PAINT_COLOR;
    shape.fill.color = nsvg_color;
    shape.stroke.type = NSVG_PAINT_NONE;
    shape.opacity = 1.0f;
    shape.fillRule = NSVG_FILLRULE_NONZERO;
    shape.flags = NSVG_FLAGS_VISIBLE;
    /* paintOrder = packed 3x2-bit slots. Default 0 means "FILL FILL
     * FILL" — the rasterizer runs the fill branch three times for
     * the same identical shape, doing 3x the work. Slot 0 = FILL,
     * slots 1 and 2 = MARKERS (which the rasterizer ignores) gives
     * one fill pass. */
    shape.paintOrder = (unsigned char)
        (NSVG_PAINT_FILL | (NSVG_PAINT_MARKERS << 2) | (NSVG_PAINT_MARKERS << 4));
    shape.paths = &path;
    shape.bounds[0] = path.bounds[0];
    shape.bounds[1] = path.bounds[1];
    shape.bounds[2] = path.bounds[2];
    shape.bounds[3] = path.bounds[3];

    /* Tight bounding box for the temp RGBA buffer. Pad by 1px on each
     * side so the AA fringe at edge pixels has somewhere to land. */
    int bbx0 = (int)floorf(path.bounds[0]) - 1;
    int bby0 = (int)floorf(path.bounds[1]) - 1;
    int bbx1 = (int)ceilf(path.bounds[2])  + 1;
    int bby1 = (int)ceilf(path.bounds[3])  + 1;
    int W = bbx1 - bbx0;
    int H = bby1 - bby0;
    if (W <= 0 || H <= 0) {
        efree(pts_buf);
        return;
    }

    NSVGimage img;
    memset(&img, 0, sizeof(img));
    img.width = (float)W;
    img.height = (float)H;
    img.shapes = &shape;

    int stride = W * 4;
    unsigned char *buf = ecalloc((size_t)H, (size_t)stride);
    /* tx/ty translate the image so the path's coordinates land in
     * [0..W) x [0..H) of the buffer. Scale = 1.0; we don't need any
     * vector resampling, just AA rasterization at the original
     * pixel grid. */
    nsvgRasterize(ctx->rast, &img, (float)-bbx0, (float)-bby0, 1.0f,
                  buf, W, H, stride);

    composite_rgba_onto_gd(im, buf, W, H, stride, bbx0, bby0);

    efree(buf);
    efree(pts_buf);
}

/* Filled wedge (pie slice / gauge zone) with AA edges. Build the
 * wedge as a polygon: center + sampled points along the arc. The
 * sample count scales with the arc length so visual quality is
 * preserved at any wedge size. start_deg / end_deg follow libgd's
 * convention (0 = +x, increasing CCW). */
void fastchart_aa_filled_wedge(fastchart_aa_ctx *ctx, gdImagePtr im,
                               int cx, int cy, int diameter,
                               int start_deg, int end_deg, int color)
{
    if (!ctx || diameter <= 0 || end_deg <= start_deg) return;
    int radius = diameter / 2;
    /* Sub-degree sampling: aim for ~1 sample per 2 px of arc length.
     * arc_len = r * theta (theta in radians). At r=200, full sweep =
     * ~628 px → ~314 samples. Cap at 360 for sanity. */
    int span = end_deg - start_deg;
    int n_arc = (int)((double)radius * span * M_PI / 180.0 / 2.0);
    if (n_arc < 16)  n_arc = 16;
    if (n_arc > 720) n_arc = 720;
    int n_pts = n_arc + 2;  /* arc samples + 2 for center vertex (start/end share) */
    gdPoint *poly = emalloc((size_t)n_pts * sizeof(gdPoint));
    poly[0].x = cx;
    poly[0].y = cy;
    for (int i = 0; i <= n_arc; i++) {
        double t = (double)i / (double)n_arc;
        double deg = (double)start_deg + t * (double)span;
        double rad = deg * M_PI / 180.0;
        poly[i + 1].x = cx + (int)((double)radius * cos(rad) + 0.5);
        poly[i + 1].y = cy + (int)((double)radius * sin(rad) + 0.5);
    }

    fastchart_aa_filled_polygon(ctx, im, poly, n_pts, color);
    efree(poly);
}

void fastchart_aa_stroke_line(fastchart_aa_ctx *ctx, gdImagePtr im,
                              int x1, int y1, int x2, int y2,
                              double thickness, int color)
{
    if (!ctx || !ctx->rast || thickness <= 0.0) return;

    /* Convert gd's truecolor packing to NanoSVG's alpha-low ABGR.
     * Same conversion as fastchart_aa_filled_polygon. */
    int gd_alpha = (color >> 24) & 0x7F;
    int sr = (color >> 16) & 0xFF;
    int sg = (color >>  8) & 0xFF;
    int sb =  color        & 0xFF;
    unsigned int a8 = (unsigned int)(255 - (gd_alpha * 255 / 127));
    unsigned int nsvg_color = (a8 << 24) | ((unsigned)sb << 16)
                            | ((unsigned)sg << 8)  |  (unsigned)sr;

    /* Single open path: MoveTo (x1,y1), then a degenerate cubic to
     * (x2,y2). pts layout: [x0,y0, cp1x,cp1y, cp2x,cp2y, x1,y1]. */
    float pts_buf[8];
    float fx1 = (float)x1, fy1 = (float)y1;
    float fx2 = (float)x2, fy2 = (float)y2;
    pts_buf[0] = fx1;
    pts_buf[1] = fy1;
    pts_buf[2] = fx1 + (fx2 - fx1) / 3.0f;
    pts_buf[3] = fy1 + (fy2 - fy1) / 3.0f;
    pts_buf[4] = fx1 + (fx2 - fx1) * 2.0f / 3.0f;
    pts_buf[5] = fy1 + (fy2 - fy1) * 2.0f / 3.0f;
    pts_buf[6] = fx2;
    pts_buf[7] = fy2;

    NSVGpath path;
    memset(&path, 0, sizeof(path));
    path.pts = pts_buf;
    path.npts = 4;          /* 4 NSVG points = 1 cubic segment */
    path.closed = 0;        /* open: this is a stroke, not a fill */
    path.bounds[0] = fx1 < fx2 ? fx1 : fx2;
    path.bounds[1] = fy1 < fy2 ? fy1 : fy2;
    path.bounds[2] = fx1 > fx2 ? fx1 : fx2;
    path.bounds[3] = fy1 > fy2 ? fy1 : fy2;
    path.next = NULL;

    NSVGshape shape;
    memset(&shape, 0, sizeof(shape));
    shape.fill.type = NSVG_PAINT_NONE;
    shape.stroke.type = NSVG_PAINT_COLOR;
    shape.stroke.color = nsvg_color;
    shape.opacity = 1.0f;
    shape.strokeWidth = (float)thickness;
    /* Round caps prevent visible square endpoints on diagonal needles
     * and any other line where the endpoint is mid-canvas. */
    shape.strokeLineCap  = NSVG_CAP_ROUND;
    shape.strokeLineJoin = NSVG_JOIN_ROUND;
    shape.miterLimit = 4.0f;
    shape.fillRule = NSVG_FILLRULE_NONZERO;
    shape.flags = NSVG_FLAGS_VISIBLE;
    /* paintOrder is a packed 3x2-bit field. The rasterizer iterates
     * three slots and runs fill / stroke / marker branches based on
     * which value lands in each. memset(0) leaves all three slots at
     * NSVG_PAINT_FILL — only the fill branch executes, so a
     * fill-NONE / stroke-COLOR shape would render nothing. Encode
     * STROKE in slot 0 so the stroke branch runs. */
    shape.paintOrder = (unsigned char)NSVG_PAINT_STROKE;
    shape.paths = &path;
    /* Stroke bounds extend by half the thickness in every direction. */
    float half = (float)thickness / 2.0f + 1.0f;
    shape.bounds[0] = path.bounds[0] - half;
    shape.bounds[1] = path.bounds[1] - half;
    shape.bounds[2] = path.bounds[2] + half;
    shape.bounds[3] = path.bounds[3] + half;

    int bbx0 = (int)floorf(shape.bounds[0]) - 1;
    int bby0 = (int)floorf(shape.bounds[1]) - 1;
    int bbx1 = (int)ceilf(shape.bounds[2])  + 1;
    int bby1 = (int)ceilf(shape.bounds[3])  + 1;
    int W = bbx1 - bbx0;
    int H = bby1 - bby0;
    if (W <= 0 || H <= 0) return;

    NSVGimage img;
    memset(&img, 0, sizeof(img));
    img.width = (float)W;
    img.height = (float)H;
    img.shapes = &shape;

    int stride = W * 4;
    unsigned char *buf = ecalloc((size_t)H, (size_t)stride);
    nsvgRasterize(ctx->rast, &img, (float)-bbx0, (float)-bby0, 1.0f,
                  buf, W, H, stride);
    composite_rgba_onto_gd(im, buf, W, H, stride, bbx0, bby0);
    efree(buf);
}
