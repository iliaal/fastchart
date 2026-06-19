--TEST--
BarChart: radial (circular "race track") orientation
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* setOrientation(BAR_RADIAL) draws each category as a concentric ring
 * whose bar is a thick arc swept clockwise from 12 o'clock, the peak
 * value reaching a near-full circle. Output is valid SVG with finite
 * coordinates and differs from the vertical and horizontal layouts of
 * the same data. Multiple series stack as concentric sub-bands.
 * setOrientation still rejects out-of-range values. */

function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$data = [['label' => 'sales', 'data' => [80, 55, 40, 95, 30]]];
$cats = ['Q1', 'Q2', 'Q3', 'Q4', 'Q5'];

$vert = (new FastChart\BarChart(440, 440))->setSeries($data)->setCategoryLabels($cats)
            ->setOrientation(FastChart\BarChart::BAR_VERTICAL)->renderSvg();
$radial = (new FastChart\BarChart(440, 440))->setSeries($data)->setCategoryLabels($cats)
            ->setOrientation(FastChart\BarChart::BAR_RADIAL)->renderSvg();

echo "radial_valid: ", valid($radial) ? "yes" : "no", "\n";
echo "radial_finite: ", (strpos($radial, '-2147483648') === false) ? "yes" : "no", "\n";
echo "radial_differs_from_vertical: ", ($radial !== $vert) ? "yes" : "no", "\n";
/* Radial bars are arcs (paths), not rectangles. */
echo "radial_has_arcs: ", (substr_count($radial, '<path') > 0) ? "yes" : "no", "\n";

/* Grouped: two series render and the chart stays valid. */
$grouped = (new FastChart\BarChart(460, 460))
    ->setSeries([
        ['label' => '2025', 'data' => [80, 55, 40, 95]],
        ['label' => '2026', 'data' => [60, 70, 50, 40]],
    ])
    ->setCategoryLabels(['N', 'S', 'E', 'W'])
    ->setOrientation(FastChart\BarChart::BAR_RADIAL)->renderSvg();
echo "grouped_valid: ", valid($grouped) ? "yes" : "no", "\n";

/* All-zero / no positive value throws. */
try {
    (new FastChart\BarChart(300, 300))->setSeries([['data' => [0, 0, 0]]])
        ->setOrientation(FastChart\BarChart::BAR_RADIAL)->renderSvg();
    echo "zero_throws: no\n";
} catch (\Throwable $e) {
    echo "zero_throws: yes\n";
}

/* Out-of-range orientation still rejected. */
try {
    (new FastChart\BarChart(300, 300))->setOrientation(99);
    echo "bad_orientation_throws: no\n";
} catch (\ValueError $e) {
    echo "bad_orientation_throws: yes\n";
}

echo "ok\n";
?>
--EXPECT--
radial_valid: yes
radial_finite: yes
radial_differs_from_vertical: yes
radial_has_arcs: yes
grouped_valid: yes
zero_throws: yes
bad_orientation_throws: yes
ok
