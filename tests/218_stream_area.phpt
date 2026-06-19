--TEST--
AreaChart: stream-graph (ThemeRiver) centers a multi-series stack
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* setStreamMode(true) renders a stacked area centered on the zero line
 * instead of anchored at the baseline. It draws one filled polygon per
 * series, the coordinates stay finite, and the centered silhouette differs
 * from the same data drawn as a plain (baseline-anchored) stack. */

function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$series = [
    ['data' => [1, 3, 2, 4, 2]],
    ['data' => [2, 1, 3, 1, 2]],
    ['data' => [1, 2, 1, 3, 4]],
];

$stream  = (new FastChart\AreaChart(400, 300))->setSeries($series)->setStreamMode(true)->renderSvg();
$stacked = (new FastChart\AreaChart(400, 300))->setSeries($series)->setStacked(true)->renderSvg();

echo "stream_valid: ", valid($stream) ? "yes" : "no", "\n";
echo "stream_polys_eq_3: ", (substr_count($stream, '<polygon') === 3 ? "yes" : "no"), "\n";
echo "stream_clean: ", (strpos($stream, '-2147483648') === false ? "yes" : "no"), "\n";
echo "stream_differs_from_stacked: ", ($stream !== $stacked ? "yes" : "no"), "\n";

/* Fewer than two series: stream silently falls back to the normal path. */
$one = (new FastChart\AreaChart(400, 300))
    ->setSeries([['data' => [1, 2, 3]]])->setStreamMode(true)->renderSvg();
echo "single_series_ok: ", valid($one) ? "yes" : "no", "\n";

echo "ok\n";
?>
--EXPECT--
stream_valid: yes
stream_polys_eq_3: yes
stream_clean: yes
stream_differs_from_stacked: yes
single_series_ok: yes
ok
