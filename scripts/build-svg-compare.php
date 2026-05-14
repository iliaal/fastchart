<?php
/* Build side-by-side SVG vs PNG comparison HTML. Outputs to
 * compare-svg-png.html in the repo root. Each row renders the same
 * chart through both backends; SVG is inlined as XML, PNG as a
 * base64 data URI so the HTML file is self-contained.
 *
 * Run with the fastchart + gd extensions loaded — see CLAUDE.md
 * for the explicit -d extension= invocation. */

if (!class_exists('FastChart\\LineChart')) {
    fwrite(STDERR, "FastChart not loaded; load fastchart.so + gd.so on the CLI.\n");
    exit(1);
}

$cases = [];

/* ----------- 1: minimal single series ----------- */
$c = (new FastChart\LineChart(480, 280))
    ->setSeries([12, 18, 14, 22, 19, 25, 23, 28, 24, 30])
    ->setTitle('Single series, defaults');
$cases[] = ['label' => '1. Single series, no decoration', 'chart' => $c];

/* ----------- 2: multi-series + legend + category labels ----------- */
$c = (new FastChart\LineChart(480, 280))
    ->setSeries([
        ['label' => 'alpha', 'data' => [10, 14, 12, 18, 22, 19, 25, 23]],
        ['label' => 'beta',  'data' => [ 8, 11, 15, 13, 17, 21, 20, 24]],
        ['label' => 'gamma', 'data' => [14, 16, 13, 11, 14, 18, 22, 26]],
    ])
    ->setTitle('Q3 revenue by region')
    ->setCategoryLabels(['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug']);
$cases[] = ['label' => '2. Three series, legend, category labels', 'chart' => $c];

/* ----------- 3: markers + value labels ----------- */
$c = (new FastChart\LineChart(480, 280))
    ->setSeries([23, 31, 28, 42, 38, 51, 47])
    ->setTitle('Markers + value labels')
    ->setMarkerStyle(FastChart\Chart::MARKER_DIAMOND)
    ->setMarkerSize(8)
    ->setShowValues(true);
$cases[] = ['label' => '3. Diamond markers + on-point value labels', 'chart' => $c];

/* ----------- 4: dark theme ----------- */
$c = (new FastChart\LineChart(480, 280))
    ->setSeries([
        ['label' => 'CPU',  'data' => [45, 52, 48, 67, 71, 58, 49]],
        ['label' => 'Mem',  'data' => [30, 35, 38, 41, 44, 42, 40]],
    ])
    ->setTitle('Dark theme')
    ->setTheme(FastChart\Chart::THEME_DARK);
$cases[] = ['label' => '4. Dark theme', 'chart' => $c];

/* ----------- 5: rotated x-axis labels ----------- */
$c = (new FastChart\LineChart(480, 320))
    ->setSeries([120, 145, 132, 168, 154, 189, 175, 198, 184, 215, 230, 248])
    ->setTitle('Rotated x-axis labels')
    ->setCategoryLabels(['Jan 2025', 'Feb 2025', 'Mar 2025', 'Apr 2025',
                         'May 2025', 'Jun 2025', 'Jul 2025', 'Aug 2025',
                         'Sep 2025', 'Oct 2025', 'Nov 2025', 'Dec 2025'])
    ->setXAxisLabelAngle(45);
$cases[] = ['label' => '5. X-axis labels rotated 45°', 'chart' => $c];

/* ----------- 6: horizontal annotation band + line ----------- */
$c = (new FastChart\LineChart(480, 280))
    ->setSeries([72, 78, 81, 95, 88, 105, 92, 110, 102])
    ->setTitle('Annotation band + threshold line')
    ->addHorizontalBand(90, 110, 0xFFE5E5)
    ->addHorizontalLine(100, 0xCC0000);
$cases[] = ['label' => '6. Plot band + horizontal threshold', 'chart' => $c];

/* ----------- 7: log Y scale ----------- */
$c = (new FastChart\LineChart(480, 280))
    ->setSeries([1, 4, 18, 95, 420, 1800, 9500, 48000])
    ->setTitle('Log-10 Y axis')
    ->setYAxisScale(FastChart\Chart::SCALE_LOG);
$cases[] = ['label' => '7. Log Y scale (exponential data)', 'chart' => $c];

/* ----------- 8: secondary Y axis ----------- */
$c = (new FastChart\LineChart(480, 280))
    ->setSeries([
        ['label' => 'sales ($K)', 'data' => [12, 18, 25, 30, 28, 35, 42]],
        ['label' => 'units',      'data' => [400, 520, 680, 760, 700, 880, 1020], 'axis' => 'right'],
    ])
    ->setTitle('Dual Y axes')
    ->setSecondaryYAxis(true)
    ->setYAxisTitle('USD thousands')
    ->setSecondaryYAxisTitle('units sold');
$cases[] = ['label' => '8. Secondary Y axis (right)', 'chart' => $c];

/* ----------- 9: hi-DPI (PNG is 2x; SVG ignores DPI for viewport but layout shifts) ----------- */
$c = (new FastChart\LineChart(480, 280))
    ->setSeries([22, 28, 24, 32, 35, 31, 38, 41])
    ->setTitle('Hi-DPI (200)')
    ->setCategoryLabels(['A','B','C','D','E','F','G','H'])
    ->setDpi(200);
$cases[] = ['label' => '9. setDpi(200) — PNG renders at 2x physical pixels; SVG is DPI-invariant (identical to setDpi(96))', 'chart' => $c];

/* ----------- 10: vertical bar chart, two series ----------- */
$c = (new FastChart\BarChart(480, 280))
    ->setSeries([
        ['label' => '2024', 'data' => [42, 58, 67, 71]],
        ['label' => '2025', 'data' => [55, 63, 75, 84]],
    ])
    ->setTitle('Sales by quarter')
    ->setCategoryLabels(['Q1', 'Q2', 'Q3', 'Q4']);
$cases[] = ['label' => '10. BarChart — grouped vertical bars', 'chart' => $c];

/* ----------- 11: stacked bar ----------- */
$c = (new FastChart\BarChart(480, 280))
    ->setSeries([
        ['label' => 'web',    'data' => [120, 135, 142, 158, 161]],
        ['label' => 'mobile', 'data' => [ 80, 110, 145, 170, 195]],
        ['label' => 'api',    'data' => [ 40,  55,  70,  88, 105]],
    ])
    ->setStacked(true)
    ->setTitle('Stacked bar')
    ->setCategoryLabels(['Q1', 'Q2', 'Q3', 'Q4', 'Q5']);
$cases[] = ['label' => '11. BarChart — stacked, three series', 'chart' => $c];

/* ----------- 12: pie chart ----------- */
$c = (new FastChart\PieChart(420, 280))
    ->setSlices([
        'Frontend'      => 38,
        'Backend'       => 27,
        'Database'      => 15,
        'Infrastructure' => 12,
        'Other'         =>  8,
    ])
    ->setTitle('Engineering time allocation');
$cases[] = ['label' => '12. PieChart — five slices', 'chart' => $c];

/* ----------- 13: donut variant ----------- */
$c = (new FastChart\PieChart(420, 280))
    ->setSlices([
        'A' => 30, 'B' => 25, 'C' => 20, 'D' => 15, 'E' => 10,
    ])
    ->setDonutHoleRatio(0.45)
    ->setTitle('Donut chart');
$cases[] = ['label' => '13. PieChart — donut (45% hole)', 'chart' => $c];

/* ----------- 14: stock chart with MA + volume ----------- */
$rows = [];
$base = 1700000000;
$prices = [
    [100, 110,  95, 108], [108, 112, 102, 105], [105, 115, 100, 113],
    [113, 118, 110, 111], [111, 119, 108, 117], [117, 121, 114, 116],
    [116, 120, 112, 119], [119, 122, 116, 117], [117, 124, 115, 122],
    [122, 126, 119, 125], [125, 130, 121, 128], [128, 132, 123, 127],
    [127, 133, 124, 131], [131, 135, 128, 130], [130, 138, 127, 136],
];
foreach ($prices as $i => $p) {
    $rows[] = [$base + $i * 86400, $p[0], $p[1], $p[2], $p[3], 1000 + $i * 100];
}
$c = (new FastChart\StockChart(640, 360))
    ->setOhlcv($rows)
    ->setMovingAverages([5])
    ->setVolumePane(true)
    ->setTitle('OHLC + SMA(5) + volume');
$cases[] = ['label' => '14. StockChart — candlesticks, SMA overlay, volume pane', 'chart' => $c];

/* ----------- HTML envelope ----------- */
$rows = '';
$total_svg = 0;
$total_png = 0;
foreach ($cases as $i => $case) {
    $c = $case['chart'];
    $svg = $c->renderSvg();
    $png = $c->renderPng();
    $total_svg += strlen($svg);
    $total_png += strlen($png);
    $png_b64 = 'data:image/png;base64,' . base64_encode($png);
    $svg_kb = number_format(strlen($svg) / 1024, 1);
    $png_kb = number_format(strlen($png) / 1024, 1);
    $label = htmlspecialchars($case['label'], ENT_QUOTES, 'UTF-8');
    $rows .= <<<HTML
<section class="row">
  <h2>{$label}</h2>
  <div class="pair">
    <figure>
      <figcaption>SVG <span class="size">{$svg_kb} KB</span></figcaption>
      <div class="frame">{$svg}</div>
    </figure>
    <figure>
      <figcaption>PNG (libgd) <span class="size">{$png_kb} KB</span></figcaption>
      <div class="frame"><img src="{$png_b64}" alt="PNG render"></div>
    </figure>
  </div>
</section>
HTML;
}

$total_svg_kb = number_format($total_svg / 1024, 1);
$total_png_kb = number_format($total_png / 1024, 1);
$now = date('Y-m-d H:i:s');

$html = <<<HTML
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>FastChart: SVG vs PNG comparison</title>
<style>
:root {
  --bg: #fafafa;
  --fg: #222;
  --muted: #666;
  --border: #ddd;
  --accent: #1f77b4;
}
html, body { margin: 0; padding: 0; background: var(--bg); color: var(--fg);
             font-family: -apple-system, "Segoe UI", Roboto, sans-serif; }
header { background: #fff; border-bottom: 1px solid var(--border);
         padding: 16px 24px; }
header h1 { margin: 0 0 4px; font-size: 1.4rem; }
header p { margin: 0; color: var(--muted); font-size: 0.9rem; }
header code { background: #f0f0f0; padding: 1px 5px; border-radius: 3px;
              font-size: 0.85em; }
main { max-width: 1100px; margin: 0 auto; padding: 24px; }
.row { background: #fff; border: 1px solid var(--border); border-radius: 8px;
       margin-bottom: 28px; padding: 16px 20px; }
.row h2 { margin: 0 0 12px; font-size: 1rem; font-weight: 600;
          color: var(--accent); }
.pair { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
figure { margin: 0; }
figcaption { font-size: 0.8rem; color: var(--muted); margin-bottom: 6px;
             display: flex; justify-content: space-between; }
figcaption .size { font-family: ui-monospace, "SF Mono", Menlo, monospace; }
.frame { background: #fff; border: 1px solid var(--border); border-radius: 4px;
         overflow: hidden; padding: 8px; display: flex; justify-content: center;
         align-items: center; }
.frame svg, .frame img { max-width: 100%; height: auto; display: block; }
footer { max-width: 1100px; margin: 0 auto; padding: 16px 24px 40px;
         color: var(--muted); font-size: 0.85rem; }
</style>
</head>
<body>
<header>
  <h1>FastChart: SVG vs PNG side-by-side</h1>
  <p>Each row is the same chart rendered through both backends.
     SVG is inlined; PNG via base64 data URI. Generated {$now}.
     SVG total: <code>{$total_svg_kb} KB</code> &middot;
     PNG total: <code>{$total_png_kb} KB</code></p>
</header>
<main>
{$rows}
</main>
<footer>
  Generated by <code>scripts/build-svg-compare.php</code>.
  SVG output: LineChart only in this build (PR 1 pilot). BarChart,
  PieChart, StockChart land in PRs 2-4.
</footer>
</body>
</html>
HTML;

$out = __DIR__ . '/../compare-svg-png.html';
file_put_contents($out, $html);
echo "wrote ", realpath($out), " (", number_format(filesize($out) / 1024, 1), " KB)\n";
echo "SVG total: {$total_svg_kb} KB across ", count($cases), " charts\n";
echo "PNG total: {$total_png_kb} KB across ", count($cases), " charts\n";
