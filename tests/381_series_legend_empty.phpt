--TEST--
Series legend: a labeled series always gets an entry, even when empty
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fastchart_draw_series_legend unifies legend construction across
 * Line / Area / Bar / Scatter. Policy: a labeled series always earns a
 * legend entry, even with no data. Previously LineChart skipped empty
 * series (its accumulation sat behind an `n < 1` continue), so a
 * zero-length labeled series appeared on Area/Bar/Scatter but not Line.
 * Count the label occurrences in the native-text SVG legend. */

$N = FastChart\Chart::SVG_TEXT_NATIVE;

function has_both($svg) {
    return substr_count($svg, '>alpha<') === 1
        && substr_count($svg, '>beta<') === 1;
}

$line = (new FastChart\LineChart(400, 300))->setSvgTextMode($N)
    ->setSeries([['label' => 'alpha', 'data' => [1, 2, 3]],
                 ['label' => 'beta',  'data' => []]])->renderSvg();
$area = (new FastChart\AreaChart(400, 300))->setSvgTextMode($N)
    ->setSeries([['label' => 'alpha', 'data' => [1, 2, 3]],
                 ['label' => 'beta',  'data' => []]])->renderSvg();
$bar = (new FastChart\BarChart(400, 300))->setSvgTextMode($N)
    ->setSeries([['label' => 'alpha', 'data' => [1, 2, 3]],
                 ['label' => 'beta',  'data' => []]])->renderSvg();
$scatter = (new FastChart\ScatterChart(400, 300))->setSvgTextMode($N)
    ->setPoints([['label' => 'alpha', 'data' => [[1, 1], [2, 2]]],
                 ['label' => 'beta',  'data' => []]])->renderSvg();

echo "line empty-series legend: ",   has_both($line)    ? "yes" : "no", "\n";
echo "area empty-series legend: ",   has_both($area)    ? "yes" : "no", "\n";
echo "bar empty-series legend: ",    has_both($bar)     ? "yes" : "no", "\n";
echo "scatter empty-series legend: ", has_both($scatter) ? "yes" : "no", "\n";

?>
--EXPECT--
line empty-series legend: yes
area empty-series legend: yes
bar empty-series legend: yes
scatter empty-series legend: yes
