--TEST--
svgToPng: stroke outline size is budgeted per document
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php

function dashed_round(int $n): string
{
    return '<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64">'
        . str_repeat('<line x1="0" y1="8" x2="1e6" y2="8" stroke="#000" '
            . 'stroke-linecap="round" stroke-dasharray="0.2"/>', $n)
        . '</svg>';
}

/* Each line stays inside the dash budget, but two of them stroke into
 * more outline points than the document may hold. */
try {
    FastChart\Chart::svgToPng(dashed_round(2));
    echo "over budget: rendered\n";
} catch (ValueError $e) {
    echo 'over budget: ', $e->getMessage(), "\n";
}

$png = FastChart\Chart::svgToPng(dashed_round(1));
echo 'one line: ', imagecreatefromstring($png) ? "rendered\n" : "NO\n";

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
one line: rendered
many ordinary strokes: rendered
chart: rendered
