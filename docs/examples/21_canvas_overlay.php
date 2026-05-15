<?php
/* Overlay extra graphics on top of a chart via SVG composition.
 * v1.0 retired draw($canvas), so the canonical workflow is:
 *   1. build the chart, call drawSvgFragment() to get a <g>...</g>
 *      group
 *   2. assemble an outer <svg> document with the fragment + your own
 *      overlay elements (text, shapes, embedded images)
 *   3. write the assembled SVG to disk, or rasterize it through any
 *      SVG-aware tool (Inkscape, librsvg, the user's own browser).
 *
 * This example builds a stock chart, then overlays a translucent
 * "DRAFT" watermark, a footer credit line, and a colored ring at a
 * specific data point. All overlays live in the same SVG document
 * as the chart, side-by-side with the chart group. */

require __DIR__ . '/_bootstrap.php';

mt_srand(3);
$rows = [];
$base = 1700000000;
$price = 100.0;
for ($i = 0; $i < 60; $i++) {
    $delta = (mt_rand(-100, 100) / 100.0) * 1.4;
    $open  = $price;
    $close = $price + $delta;
    $high  = max($open, $close) + mt_rand(0, 60) / 100.0;
    $low   = min($open, $close) - mt_rand(0, 60) / 100.0;
    $rows[] = [$base + $i * 86400, $open, $high, $low, $close, 1000];
    $price  = $close;
}

$W = 720; $H = 420;

/* Step 1: chart fragment. drawSvgFragment returns just the <g> group
 * so we can drop it into a larger SVG document without nested <svg>
 * elements (which not all renderers handle cleanly). */
$chart_g = (new FastChart\StockChart($W, $H))
    ->setFontPath($font)
    ->setTitle('ACME internal preview')
    ->setOhlcv($rows)
    ->addMovingAverage(20)
    ->setDateAxisStride(FastChart\Chart::DATE_WEEK, 2)
    ->drawSvgFragment();

/* Step 2: assemble the outer document. Background fill, chart group,
 * then the overlays painted on top. */
$svg  = '<?xml version="1.0" encoding="UTF-8"?>' . "\n";
$svg .= '<svg xmlns="http://www.w3.org/2000/svg" width="' . $W . '" height="' . $H
      . '" viewBox="0 0 ' . $W . ' ' . $H . '">';
$svg .= '<rect width="100%" height="100%" fill="#FFFFFF"/>';
$svg .= $chart_g;

/* Translucent "DRAFT" watermark, rotated -20 degrees and centered. */
$svg .= '<text x="' . ($W / 2) . '" y="' . ($H / 2)
      . '" font-family="sans-serif" font-size="84" font-weight="bold"'
      . ' fill="#C81E1E" fill-opacity="0.18"'
      . ' text-anchor="middle" dominant-baseline="middle"'
      . ' transform="rotate(-20 ' . ($W / 2) . ' ' . ($H / 2) . ')">DRAFT</text>';

/* Footer credit, bottom-left. */
$svg .= '<text x="8" y="' . ($H - 8) . '"'
      . ' font-family="sans-serif" font-size="11" fill="#646464">'
      . 'Generated ' . date('Y-m-d') . ' (confidential)</text>';

/* Highlight ring near the right edge — coordinates chosen relative
 * to the canvas, not the chart's data space, since drawSvgFragment
 * doesn't expose pixel positions of individual data points. */
$rx = $W - 60; $ry = 200;
$svg .= '<circle cx="' . $rx . '" cy="' . $ry . '" r="12"'
      . ' fill="none" stroke="#FFDC00" stroke-width="2"/>';
$svg .= '<circle cx="' . $rx . '" cy="' . $ry . '" r="13"'
      . ' fill="none" stroke="#FFDC00" stroke-width="1"/>';

$svg .= '</svg>';
file_put_contents(__DIR__ . '/21_canvas_overlay.svg', $svg);

/* For a PNG: render the chart directly, then assemble a composite
 * image via any external rasterizer. The fastchart-side step is the
 * .svg above; the .png path below is a chart-only render for the
 * cookbook image. */
(new FastChart\StockChart($W, $H))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('ACME internal preview')
    ->setOhlcv($rows)
    ->addMovingAverage(20)
    ->setDateAxisStride(FastChart\Chart::DATE_WEEK, 2)
    ->renderToFile(__DIR__ . '/21_canvas_overlay.png');
