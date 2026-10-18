--TEST--
Value-label budget keeps a draw-order prefix, leaves default labels intact, and throws in strict mode
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
$c = (new FastChart\LineChart(600,400))->setSeries([1,2])->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
if (!str_contains($c->renderSvg(), '<text ')) echo 'skip no usable default font';
?>
--FILE--
<?php
use FastChart\Chart;

function chart(string $fmt, int $n = 2048, float $v = 1e308): FastChart\LineChart {
    $c = (new FastChart\LineChart(800, 600))
        ->setSeries(array_fill(0, $n, $v))
        ->setSvgTextMode(Chart::SVG_TEXT_NATIVE);
    return $fmt === '' ? $c->setShowValues(true) : $c->setShowValues(true, $fmt);
}

$svg = chart('')->renderSvg();
echo "default labels: ", substr_count($svg, '>1.0e+308</text>'), "\n";

$svg = chart('%f')->renderSvg();
preg_match_all('/<text\b[^>]*>(\d{300,}\.\d{6})<\/text>/', $svg, $m);
$n = count($m[1]);
$len = strlen($m[1][0] ?? '');
echo "%f labels: ", $n > 0 && $n * $len <= 65536 && ($n + 1) * $len > 65536
    ? "prefix fills the budget\n" : "n=$n len=$len\n";

try {
    chart('%f')->setStrict(true)->renderSvg();
    echo "strict: no exception\n";
} catch (ValueError $e) {
    echo "strict: ", $e->getMessage(), "\n";
}

$fmt = str_repeat('x', 8000) . '%.0f';
$svg = chart($fmt, 8, 5)->setStrict(true)->renderSvg();
echo "strict at budget: ", substr_count($svg, '>' . str_repeat('x', 8000)), "\n";
?>
--EXPECT--
default labels: 2048
%f labels: prefix fills the budget
strict: FastChart: per-element labels exceed the 65536-byte per-render text limit
strict at budget: 8
