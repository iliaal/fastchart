--TEST--
ScatterChart X tick labels honor the setAxisFont() size, not the base font size
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The hand-rolled scatter X axis resolved the axis font path but drew
 * labels at the raw base font_size, ignoring the size passed to
 * setAxisFont(). The shared numeric-axis drawer resolves both. In native
 * text mode the emitted size is 40 * 4/3 = 53.3 (derived from the pt
 * value, so no real font file is needed). The Y axis is hidden so only
 * the X tick labels remain — otherwise the Y labels (which already honor
 * the axis size) would mask the X-axis bug. */

$svg = (new FastChart\ScatterChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setPoints([[0, 0], [5, 10], [9, 3]])
    ->setAxisFont('/no/such/font.ttf', 40.0)
    ->setYAxisVisible(false)
    ->renderSvg();

echo "x labels use axis size (53.3): ",
    strpos($svg, 'font-size="53.3"') !== false ? "yes" : "no", "\n";
echo "x labels not at base size (13.3): ",
    strpos($svg, 'font-size="13.3"') === false ? "yes" : "no", "\n";

?>
--EXPECT--
x labels use axis size (53.3): yes
x labels not at base size (13.3): yes
