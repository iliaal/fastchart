--TEST--
VectorChart tick labels use the axis font and are suppressed in thumbnail mode
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression (fnd_3a229545, fnd_42714279): VectorChart hand-rolls its axis
 * drawing. It resolved tick labels with the LABEL font role instead of AXIS
 * (so setAxisFont was ignored) and never checked thumbnail mode (so labels
 * rendered in thumbnails, unlike every other chart). */

$vecs = [];
for ($x = 0; $x < 4; $x++)
    for ($y = 0; $y < 4; $y++)
        $vecs[] = ['x' => $x, 'y' => $y, 'dx' => 0.3, 'dy' => 0.3];

/* Axis font role: setAxisFont(40) must reach the tick labels. In native
 * text mode the emitted font-size is 40 * 4/3 = 53.3 (size is computed
 * from the pt value; the file is never opened, so any existing regular
 * file satisfies the setter's existence gate). Before the fix the
 * axis size was ignored and labels stayed at the ~13.3 default. */
$c = (new FastChart\VectorChart(400, 400))
    ->setVectors($vecs)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setAxisFont(__FILE__, 40.0);
$svg = $c->renderSvg();
echo "axis font reaches labels: ", strpos($svg, 'font-size="53.3"') !== false ? "yes" : "no", "\n";

/* Thumbnail mode suppresses tick labels; normal mode draws them. */
$n_normal = substr_count((new FastChart\VectorChart(400, 400))->setVectors($vecs)->renderSvg(), '<path');
$n_thumb  = substr_count((new FastChart\VectorChart(400, 400))->setVectors($vecs)
                ->setThumbnailMode(true)->renderSvg(), '<path');
echo "thumbnail drops labels: ", $n_thumb < $n_normal ? "yes" : "no", "\n";

?>
--EXPECT--
axis font reaches labels: yes
thumbnail drops labels: yes
