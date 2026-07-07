--TEST--
ScatterChart::renderSvg() is DPI-invariant (setDpi does not scale SVG output)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* SVG output is documented DPI-invariant: the viewport stays at the
 * logical setSize() value and vector strokes scale infinitely, so
 * setDpi() must not change a byte. The scatter axis used to hand-roll
 * its ticks with a raw dpi/96 scale even for SVG targets, breaking the
 * contract; delegating to the shared numeric-axis drawer (which returns
 * chart_dpi_scale()==1.0 for SVG) restores it. Line charts already
 * behave this way. */

$mk = function (int $dpi): string {
    $c = (new FastChart\ScatterChart(400, 300))
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setPoints([[0, 0], [5, 10], [9, 3]]);
    if ($dpi > 0) {
        $c->setDpi($dpi);
    }
    return $c->renderSvg();
};

$base = $mk(0);
echo "dpi96 == dpi200: ",  $base === $mk(200) ? "yes" : "no", "\n";
echo "dpi96 == dpi300: ",  $base === $mk(300) ? "yes" : "no", "\n";

?>
--EXPECT--
dpi96 == dpi200: yes
dpi96 == dpi300: yes
