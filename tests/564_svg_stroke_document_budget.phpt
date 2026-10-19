--TEST--
svgToPng: stroke outline size is budgeted per document
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php

/* Each stroked cubic with control points far off a small canvas splits
 * into ~180 outline points. */
function curves(int $n): string
{
    $d = 'M0 0';
    for ($i = 0; $i < $n; $i++) {
        $d .= sprintf(' C 3000 -3000 -3000 3000 %d %d', $i % 64, $i % 37);
    }
    return '<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64">'
        . '<path d="' . $d . '" fill="none" stroke="#000"/></svg>';
}

/* ~5.4M points: over the 2^22 document budget. */
try {
    FastChart\Chart::svgToPng(curves(30000));
    echo "over budget: rendered\n";
} catch (ValueError $e) {
    echo 'over budget: ', $e->getMessage(), "\n";
}

/* ~1M points, about a quarter of the budget, still renders. */
$png = FastChart\Chart::svgToPng(curves(5500));
echo 'heavy but within budget: ', imagecreatefromstring($png) ? "rendered\n" : "NO\n";

/* Ordinary stroke-heavy documents stay far below the budget. */
$paths = '';
for ($i = 0; $i < 400; $i++) {
    $paths .= sprintf('<path d="M%d 0 C 10 300 300 10 %d 400" fill="none" '
        . 'stroke="#036" stroke-width="2" stroke-linejoin="round"/>',
        $i, 400 - $i);
}
$png = FastChart\Chart::svgToPng(
    '<svg xmlns="http://www.w3.org/2000/svg" width="400" height="400">'
    . $paths . '</svg>');
echo 'many ordinary strokes: ', imagecreatefromstring($png) ? "rendered\n" : "NO\n";

/* Chart renders carry no stroke budget. */
$chart = (new FastChart\LineChart(640, 360))
    ->setSeries(array_map(fn($i) => sin($i / 40) * 50, range(0, 2047)));
echo 'chart: ', imagecreatefromstring($chart->renderPng()) ? "rendered\n" : "NO\n";

?>
--EXPECT--
over budget: FastChart\Chart::svgToPng() SVG render work exceeds cap (stroke outlines too large)
heavy but within budget: rendered
many ordinary strokes: rendered
chart: rendered
