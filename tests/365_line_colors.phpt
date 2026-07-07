--TEST--
LineChart setSeriesColors / setBackgroundColor / setPlotBackgroundColor land on the right elements
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: these three setters had zero references. Colours
 * serialise as uppercase hex, so each must appear on its own element
 * kind: the canvas background rect (full width/height), the plot
 * background rect, and the series polyline stroke. Distinct values
 * keep the three unambiguous. */

$svg = (new FastChart\LineChart(400, 300))
    ->setSeries([10, 20, 15, 30, 25])
    ->setSeriesColors([0xAB12CD])
    ->setBackgroundColor(0x123456)
    ->setPlotBackgroundColor(0x654321)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->renderSvg();

/* Canvas background: full-size rect at the origin. */
$bg = preg_match('/<rect x="0" y="0" width="400" height="300" fill="#123456"\/>/', $svg) === 1;
/* Plot background: an inset rect with the plot colour. */
$plot = preg_match('/<rect x="[1-9][0-9]*" y="[1-9][0-9]*" width="[0-9]+" height="[0-9]+" fill="#654321"\/>/', $svg) === 1;
/* Series polyline: 2px stroke in the series colour. */
$series = strpos($svg, 'stroke="#AB12CD" stroke-width="2"') !== false;

echo "background: ", $bg ? 'yes' : 'no', "\n";
echo "plot_background: ", $plot ? 'yes' : 'no', "\n";
echo "series_stroke: ", $series ? 'yes' : 'no', "\n";
/* Data-point markers carry the same series colour as a fill (one per
 * point); the backgrounds never borrow the series colour. */
echo "markers: ", preg_match_all('/<circle [^>]*fill="#AB12CD"\/>/', $svg), "\n";
?>
--EXPECT--
background: yes
plot_background: yes
series_stroke: yes
markers: 5
