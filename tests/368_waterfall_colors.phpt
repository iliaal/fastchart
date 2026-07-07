--TEST--
Waterfall::setFallColor() and setTotalColor() paint the fall and total bars
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: setFallColor() and setTotalColor() had zero
 * references. A negative delta bar takes the fall colour, a
 * 'total' bar takes the total colour; both appear as rect fills
 * (the outline rect is fill="none"). Distinct hex values keep the
 * two unambiguous. Rise colour is set too so a wrong routing would
 * mislabel the negative bar. */

$svg = (new FastChart\Waterfall(500, 300))
    ->setBars([
        ['label' => 'A', 'value' => 50,  'kind' => 'delta'],
        ['label' => 'B', 'value' => -20, 'kind' => 'delta'],
        ['label' => 'T', 'value' => 30,  'kind' => 'total'],
    ])
    ->setRiseColor(0x1100FA)
    ->setFallColor(0xFA1100)
    ->setTotalColor(0x00FA11)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->renderSvg();

/* Each bar's body is a filled rect; the fall/total colours must each
 * appear as a real fill (not on a fill="none" outline). */
echo "fall_filled: ",  (strpos($svg, 'fill="#FA1100"') !== false ? 'yes' : 'no'), "\n";
echo "total_filled: ", (strpos($svg, 'fill="#00FA11"') !== false ? 'yes' : 'no'), "\n";
/* The rise colour belongs to the positive delta bar, not the fall/total. */
echo "rise_present: ",  (strpos($svg, 'fill="#1100FA"') !== false ? 'yes' : 'no'), "\n";
/* Exactly one filled bar per colour. */
echo "fall_once: ",  (substr_count($svg, 'fill="#FA1100"') === 1 ? 'yes' : 'no'), "\n";
echo "total_once: ", (substr_count($svg, 'fill="#00FA11"') === 1 ? 'yes' : 'no'), "\n";
?>
--EXPECT--
fall_filled: yes
total_filled: yes
rise_present: yes
fall_once: yes
total_once: yes
