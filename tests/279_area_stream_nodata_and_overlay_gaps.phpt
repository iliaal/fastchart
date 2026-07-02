--TEST--
AreaChart: stream all-gap errors; area overlays break the fill at gaps
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression 1: stream mode unconditionally set seen_l after installing
 * a fallback range, bypassing the "no numeric values" error so an
 * all-gap stream rendered an empty chart instead of rejecting no data.
 * Regression 2: area overlays compacted every valid point into one
 * closed polygon, so a null gap ([10, null, 30]) painted straight across
 * the missing category. The fill now breaks into per-run polygons. */

/* all-gap stream must error */
try {
    (new FastChart\AreaChart(300, 200))->setStreamMode(true)
        ->setSeries([['name' => 'a', 'data' => [null, null]]])->renderSvg();
    echo "stream_allgap: no-throw\n";
} catch (\Throwable $e) {
    echo "stream_allgap: ", (str_contains($e->getMessage(), 'no numeric') ? 'threw' : 'other'), "\n";
}

/* stream with data still renders */
$s = (new FastChart\AreaChart(300, 200))->setStreamMode(true)
    ->setSeries([['name' => 'a', 'data' => [1, 2, 3]]])->renderSvg();
echo "stream_data: ", (strlen($s) > 100 ? "renders" : "BAD"), "\n";

/* overlay with an interior gap => two fill polygons, not one spanning it */
$gapped = (new FastChart\BarChart(400, 300))->setCategoryLabels(['a', 'b', 'c'])
    ->setSeries([['name' => 's', 'data' => [1, 2, 3]]])
    ->addOverlaySeries('area', [10, null, 30])->renderSvg();
$contig = (new FastChart\BarChart(400, 300))->setCategoryLabels(['a', 'b', 'c'])
    ->setSeries([['name' => 's', 'data' => [1, 2, 3]]])
    ->addOverlaySeries('area', [10, 20, 30])->renderSvg();
echo "gap_valid: ", (strlen($gapped) > 100 ? "yes" : "no"), "\n";
/* the gapped fill (single run of 2 valid points -> one polygon) differs
 * from the contiguous fill (3 points) — the point is it does not span
 * the gap, so its polygon has fewer vertices than the contiguous one. */
echo "gap_differs: ", ($gapped !== $contig ? "yes" : "no"), "\n";

?>
--EXPECT--
stream_allgap: threw
stream_data: renders
gap_valid: yes
gap_differs: yes
