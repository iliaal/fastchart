--TEST--
ParetoChart::setLineColor() strokes the cumulative-% overlay line
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: setLineColor() had zero references. The cumulative-%
 * overlay is drawn as 2px line segments (plus marker circles) in the
 * chosen colour; the bars are not, so the colour appears only on the
 * overlay. */

$svg = (new FastChart\ParetoChart(500, 300))
    ->setBars([
        ['label' => 'a', 'value' => 50],
        ['label' => 'b', 'value' => 30],
        ['label' => 'c', 'value' => 20],
    ])
    ->setLineColor(0xEE0055)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->renderSvg();

/* The cumulative line is a 2px stroke in the line colour. */
echo "line_stroked: ", (strpos($svg, 'stroke="#EE0055" stroke-width="2"') !== false ? 'yes' : 'no'), "\n";
/* Two segments join the three cumulative points. */
echo "segments: ", substr_count($svg, 'stroke="#EE0055" stroke-width="2"'), "\n";
/* One marker circle per cumulative point, filled in the same colour. */
echo "markers: ", preg_match_all('/<circle [^>]*fill="#EE0055"\/>/', $svg), "\n";
?>
--EXPECT--
line_stroked: yes
segments: 2
markers: 3
