--TEST--
setTheme / setLegendPosition / setMarkerStyle: rendered effect + validation
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: the theme, legend-corner, and marker-shape enums had
 * setter smoke tests but nothing asserting they change the rendered
 * output — a regression that ignored the stored value would pass. */

use FastChart\Chart;
use FastChart\LineChart;

require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();
if ($font === '') die("skip no system font available\n");

function base(): LineChart {
    global $font;
    return (new LineChart(400, 300))
        ->setSvgTextMode(Chart::SVG_TEXT_NATIVE)
        ->setFontPath($font)
        ->setSeries([
            ['label' => 'alpha', 'data' => [1, 5, 3, 8]],
            ['label' => 'beta',  'data' => [2, 4, 6, 3]],
        ]);   /* two series: LineChart draws a legend only for >= 2 */
}

/* Themes produce different documents; dark background is dark. */
$light = base()->setTheme(Chart::THEME_LIGHT)->renderSvg();
$dark  = base()->setTheme(Chart::THEME_DARK)->renderSvg();
echo "themes_differ: ", ($light !== $dark ? 'yes' : 'NO'), "\n";
preg_match('/<rect[^>]*fill="#([0-9a-fA-F]{6})"/', $dark, $m);
$lum = hexdec(substr($m[1], 0, 2)) + hexdec(substr($m[1], 2, 2)) + hexdec(substr($m[1], 4, 2));
echo "dark_bg_is_dark: ", ($lum < 3 * 128 ? 'yes' : "NO (#{$m[1]})"), "\n";

try {
    base()->setTheme(7);
    echo "bad_theme: NO THROW\n";
} catch (ValueError $e) {
    echo "bad_theme: throws\n";
}

/* Legend: NONE omits the series label; the four corners each draw it
 * and produce four distinct layouts. */
$none = base()->setLegendPosition(Chart::LEGEND_NONE)->renderSvg();
echo "legend_none_no_label: ", (!str_contains($none, 'alpha') ? 'yes' : 'NO'), "\n";

$corners = [];
foreach ([Chart::LEGEND_TOP_RIGHT, Chart::LEGEND_TOP_LEFT,
          Chart::LEGEND_BOTTOM_RIGHT, Chart::LEGEND_BOTTOM_LEFT] as $pos) {
    $svg = base()->setLegendPosition($pos)->renderSvg();
    if (!str_contains($svg, 'alpha')) {
        echo "corner_$pos: label missing\n";
    }
    $corners[] = $svg;
}
echo "corners_distinct: ", (count(array_unique($corners)) === 4 ? 'yes' : 'NO'), "\n";

try {
    base()->setLegendPosition(9);
    echo "bad_legend: NO THROW\n";
} catch (ValueError $e) {
    echo "bad_legend: throws\n";
}

/* Markers: every shape differs from MARKER_NONE and from each other;
 * circle adds ellipse/circle elements, square adds rects. */
$plain = base()->setMarkerStyle(Chart::MARKER_NONE)->renderSvg();
$shapes = [];
foreach (['circle' => Chart::MARKER_CIRCLE, 'square' => Chart::MARKER_SQUARE,
          'diamond' => Chart::MARKER_DIAMOND, 'cross' => Chart::MARKER_CROSS,
          'plus' => Chart::MARKER_PLUS] as $name => $style) {
    $svg = base()->setMarkerStyle($style)->setMarkerSize(6)->renderSvg();
    if ($svg === $plain) {
        echo "marker_$name: no effect\n";
    }
    $shapes[$name] = $svg;
}
echo "marker_shapes_distinct: ",
    (count(array_unique($shapes)) === 5 ? 'yes' : 'NO'), "\n";

$circ = substr_count($shapes['circle'], '<ellipse') + substr_count($shapes['circle'], '<circle');
$circ0 = substr_count($plain, '<ellipse') + substr_count($plain, '<circle');
echo "circle_adds_ellipses: ", ($circ >= $circ0 + 4 ? 'yes' : 'NO'), "\n";
$rects = substr_count($shapes['square'], '<rect');
$rects0 = substr_count($plain, '<rect');
echo "square_adds_rects: ", ($rects >= $rects0 + 4 ? 'yes' : 'NO'), "\n";

try {
    base()->setMarkerStyle(42);
    echo "bad_marker: NO THROW\n";
} catch (ValueError $e) {
    echo "bad_marker: throws\n";
}

?>
--EXPECT--
themes_differ: yes
dark_bg_is_dark: yes
bad_theme: throws
legend_none_no_label: yes
corners_distinct: yes
bad_legend: throws
marker_shapes_distinct: yes
circle_adds_ellipses: yes
square_adds_rects: yes
bad_marker: throws
