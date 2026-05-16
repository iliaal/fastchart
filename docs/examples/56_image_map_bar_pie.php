<?php
/* Chart::setImageMap() + Chart::getImageMap(): produces an HTML
 * <map> with per-data-point hot-spots that the browser turns into
 * clickable / hover-tooltipped regions over the rendered raster.
 *
 *   - BarChart: rect hot-spot per category column
 *   - PieChart: poly hot-spot per slice (6-vertex wedge)
 *   - ScatterChart: circle hot-spot per point (legacy: via setPoints
 *                   entries with 'href' / 'tooltip' keys)
 *
 * URL scheme is allowlisted (http / https / mailto / relative `/`
 * or fragment `#` only — javascript:, data:, vbscript:, etc. are
 * silently dropped). Tooltip text is HTML-escaped. NUL bytes are
 * rejected. The map name is sanitized to [A-Za-z0-9_-]. */

require __DIR__ . '/_bootstrap.php';

/* BarChart: quarterly revenue with drill-through URLs. */
$bar = (new FastChart\BarChart(640, 360))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Quarterly revenue')
    ->setSeries([
        ['label' => 'revenue ($M)', 'data' => [12.4, 18.1, 21.7, 25.3]],
    ])
    ->setCategoryLabels(['Q1', 'Q2', 'Q3', 'Q4'])
    ->setImageMap([
        ['href' => '/reports/2026q1', 'tooltip' => 'Q1: $12.4M'],
        ['href' => '/reports/2026q2', 'tooltip' => 'Q2: $18.1M'],
        ['href' => '/reports/2026q3', 'tooltip' => 'Q3: $21.7M'],
        ['href' => '/reports/2026q4', 'tooltip' => 'Q4: $25.3M'],
    ]);

/* getImageMap MUST run after a render — the renderer populates the
 * hot-spot geometry as it draws each bar. */
$bar->renderToFile(__DIR__ . '/56_image_map_bar.png');
$bar_map = $bar->getImageMap('quarterly');

/* PieChart: slice click-throughs to a per-channel page. */
$pie = (new FastChart\PieChart(420, 420))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Traffic source')
    ->setSlices([
        'Organic'   => 42,
        'Paid'      => 24,
        'Referral'  => 18,
        'Email'     =>  9,
        'Direct'    =>  7,
    ])
    ->setImageMap([
        ['href' => '/channels/organic',  'tooltip' => 'Organic 42%'],
        ['href' => '/channels/paid',     'tooltip' => 'Paid 24%'],
        ['href' => '/channels/referral', 'tooltip' => 'Referral 18%'],
        ['href' => '/channels/email',    'tooltip' => 'Email 9%'],
        ['href' => '/channels/direct',   'tooltip' => 'Direct 7%'],
    ]);
$pie->renderToFile(__DIR__ . '/56_image_map_pie.png');
$pie_map = $pie->getImageMap('channels');

/* Emit a tiny standalone HTML page that wires the two maps. */
$html = <<<HTML
<!doctype html>
<meta charset="utf-8">
<title>fastchart image-map demo</title>
<style>body{font:14px/1.4 system-ui;margin:2em}h2{margin-top:2em}</style>
<h1>fastchart image-map demo</h1>
<h2>Quarterly revenue (rect hot-spots)</h2>
<img src="56_image_map_bar.png" usemap="#quarterly" alt="quarterly revenue">
$bar_map
<h2>Traffic source (poly hot-spots)</h2>
<img src="56_image_map_pie.png" usemap="#channels" alt="traffic source">
$pie_map
HTML;
file_put_contents(__DIR__ . '/56_image_map.html', $html);
