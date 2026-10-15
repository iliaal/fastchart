--TEST--
Heatmap measures complete formatted cell labels before deciding whether they fit
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
$chart = (new FastChart\Heatmap(200, 200))->setGrid([[0, 1]])
    ->setShowValues(true)->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
if (!str_contains($chart->renderSvg(), '<text ')) echo 'skip no usable default font';
?>
--FILE--
<?php
use FastChart\Chart;
use FastChart\Heatmap;

function labels(string $svg): array {
    preg_match_all('/<text\b[^>]*>([^<]*)<\/text>/', $svg, $matches);
    return $matches[1];
}
$chart = (new Heatmap(6000, 400))->setGrid([[50, NAN], [-25, 0]])
    ->setSvgTextMode(Chart::SVG_TEXT_NATIVE);
$hidden = $chart->setShowValues(false)->renderSvg();
$default = $chart->setShowValues(true, '')->renderSvg();
foreach (['v%30.0f', 'v%31.0f', 'v%32.0f', 'v%100.2f', 'v%.300f', 'v<%.2f>&%%'] as $format) {
    $svg = $chart->setShowValues(true, $format)->renderSvg();
    $expected = [];
    foreach ([50, -25, 0] as $value) {
        // PHP sprintf caps precision at 53; accepted C formats do not.
        $text = $format === 'v%.300f' ? 'v' . $value . '.' . str_repeat('0', 300)
            : sprintf($format, $value);
        $expected[] = htmlspecialchars($text, ENT_QUOTES | ENT_XML1, 'UTF-8');
    }
    $ok = labels($svg) === $expected;
    $ok = $ok && $svg === $chart->renderSvg() && $svg === (clone $chart)->renderSvg();
    $ok = $ok && $hidden === $chart->setShowValues(false)->renderSvg();
    $ok = $ok && $svg === $chart->setShowValues(true, null)->renderSvg();
    $ok = $ok && $default === $chart->setShowValues(true, '')->renderSvg();
    echo $format, ': ', $ok ? 'complete and stable' : 'FAIL', "\n";
}
// Long labels must not display misleading prefixes when only those prefixes fit.
$chart = (new Heatmap(600, 400))->setGrid([[50, 25]])
    ->setSvgTextMode(Chart::SVG_TEXT_NATIVE);
foreach (['v%31.0f', 'v%32.0f'] as $format) {
    $expected = array_map(fn($v) => sprintf($format, $v), [50, 25]);
    echo $format, ': ', labels($chart->setShowValues(true, $format)->renderSvg()) === $expected
        ? 'narrow complete' : 'FAIL', "\n";
}
foreach (['v%100.2f', 'v%.300f'] as $format) {
    echo $format, ': ', labels($chart->setShowValues(true, $format)->renderSvg()) === []
        ? 'too wide omitted' : 'FAIL', "\n";
}
// Locate the width transition without assuming the system font's glyph metrics.
$chart = (new Heatmap(2000, 400))->setGrid([[0.1], [0.2]])
    ->setShowValues(true, 'v%100.0f')->setSvgTextMode(Chart::SVG_TEXT_NATIVE);
$expected = array_fill(0, 2, sprintf('v%100.0f', 0));
$lo = 1;
$hi = 2000;
while ($lo < $hi) {
    $mid = intdiv($lo + $hi, 2);
    $actual = labels($chart->setPlotRect(0, 0, $mid - 1, 399)->renderSvg());
    if ($actual === $expected) $hi = $mid;
    else $lo = $mid + 1;
}
foreach ([-1, 0, 1] as $delta) {
    $actual = labels($chart->setPlotRect(0, 0, $lo + $delta - 1, 399)->renderSvg());
    echo "text fit $delta: ", $actual === ($delta < 0 ? [] : $expected) ? 'correct' : 'FAIL', "\n";
}
// Complete multiline labels use the same line layout as equivalent short text.
$chart = (new Heatmap(600, 400))->setGrid([[0, 1]])->setLabelFont(null, 10)
    ->setSvgTextMode(Chart::SVG_TEXT_NATIVE);
$svg = $chart->setShowValues(true, str_repeat("v\n", 16) . '%.0f')->renderSvg();
$expected = array_merge(array_fill(0, 16, 'v'), ['0'], array_fill(0, 16, 'v'), ['1']);
echo 'multiline: ', labels($svg) === $expected ? 'complete' : 'FAIL', "\n";
// Blank intermediate lines give a <=31-byte reference with the same line count.
$reference = $chart->setShowValues(true, 'v' . str_repeat("\n", 16) . '%.0f')->renderSvg();
preg_match_all('/<text\b[^>]*>[^<]*<\/text>/', $svg, $full);
preg_match_all('/<text\b[^>]*>[^<]*<\/text>/', $reference, $short);
echo 'multiline placement: ', [$full[0][0], $full[0][16], $full[0][17], $full[0][33]] === $short[0]
    ? 'matches short reference' : 'FAIL', "\n";
// The early minimum-size guards still apply at their exact integer boundaries.
$chart = (new Heatmap(200, 200))->setGrid([[0, 1]])->setLabelFont(null, 10)
    ->setShowValues(true)->setSvgTextMode(Chart::SVG_TEXT_NATIVE);
foreach ([[24, 40, []], [25, 40, ['0', '1']], [26, 40, ['0', '1']],
          [40, 15, []], [40, 16, ['0', '1']], [40, 17, ['0', '1']]] as [$w, $h, $expected]) {
    $svg = $chart->setPlotRect(0, 0, 2 * $w - 1, $h - 1)->renderSvg();
    echo "cell $w x $h: ", labels($svg) === $expected ? 'correct' : 'FAIL', "\n";
}
?>
--EXPECT--
v%30.0f: complete and stable
v%31.0f: complete and stable
v%32.0f: complete and stable
v%100.2f: complete and stable
v%.300f: complete and stable
v<%.2f>&%%: complete and stable
v%31.0f: narrow complete
v%32.0f: narrow complete
v%100.2f: too wide omitted
v%.300f: too wide omitted
text fit -1: correct
text fit 0: correct
text fit 1: correct
multiline: complete
multiline placement: matches short reference
cell 24 x 40: correct
cell 25 x 40: correct
cell 26 x 40: correct
cell 40 x 15: correct
cell 40 x 16: correct
cell 40 x 17: correct
