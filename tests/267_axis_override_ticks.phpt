--TEST--
setYAxisRange: no out-of-range tick labels; fine intervals stride instead of truncating
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Review fixes: the range override kept nice-ticks outside the forced
 * bounds, which the pixel mapping clamped onto the plot edge with the
 * wrong label ("0" drawn at 0.3's position); an interval finer than
 * (max-min)/15 filled the 16-tick ladder and stopped, packing every
 * gridline into the bottom of the plot. */

use FastChart\LineChart;

function tick_labels(string $svg): array {
    /* NATIVE text mode keeps labels as text nodes. */
    preg_match_all('/<text[^>]*>([-0-9.]+)<\/text>/', $svg, $m);
    return $m[1];
}

$svg = (new LineChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([1, 5, 9])
    ->setCategoryLabels(['a', 'b', 'c'])   /* keep x labels non-numeric */
    ->setYAxisRange(0.3, 9.7)
    ->renderSvg();
$labels = tick_labels($svg);
$out_of_range = array_filter($labels, fn($l) => (float)$l < 0.3 - 1e-9 || (float)$l > 9.7 + 1e-9);
echo "labels_present: ", count($labels) > 0 ? 'yes' : 'NO', "\n";
echo "out_of_range_labels: ", count($out_of_range) === 0 ? 'none' : implode(',', $out_of_range), "\n";

/* Fine interval: 0..100 step 1 would need 101 ticks; the ladder must
 * span the whole range (strided), not stop at 15. */
$svg2 = (new LineChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([5, 50, 95])
    ->setCategoryLabels(['a', 'b', 'c'])
    ->setYAxisRange(0.0, 100.0, 1.0)
    ->renderSvg();
$labels2 = array_map('floatval', tick_labels($svg2));
echo "stride_spans_range: ", (max($labels2) >= 90.0 ? 'yes' : 'NO (max=' . max($labels2) . ')'), "\n";
echo "stride_tick_count_sane: ", (count($labels2) >= 2 && count($labels2) <= 20 ? 'yes' : 'NO'), "\n";

?>
--EXPECT--
labels_present: yes
out_of_range_labels: none
stride_spans_range: yes
stride_tick_count_sane: yes
