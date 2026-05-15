--TEST--
BulletChart: bands + performance bar + target tick
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

$svg = (new FastChart\BulletChart(400, 80))
    ->setRange(0, 100)
    ->setBands([
        ['from' => 0,  'to' => 60],
        ['from' => 60, 'to' => 85],
        ['from' => 85, 'to' => 100],
    ])
    ->setValue(72)
    ->setTarget(85)
    ->renderSvg();

/* Background, 3 band rects, performance rect (filled+stroke), border —
 * count rects loosely; we mostly want to know we got a coherent draw. */
echo "rects_ge_5: ", (substr_count($svg, '<rect') >= 5 ? "yes" : "no"), "\n";
echo "has_line: ", (str_contains($svg, '<line') ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Target NAN is legal (default / clear sentinel). */
$svg2 = (new FastChart\BulletChart(400, 80))
    ->setRange(0, 100)
    ->setValue(50)
    ->setTarget(NAN)
    ->renderSvg();
echo "no_target_renders: ", (strlen($svg2) > 100 ? "yes" : "no"), "\n";

/* Range validation. */
try {
    (new FastChart\BulletChart(400, 80))->setRange(10, 5);
    echo "bad_range_accepted: REGRESSION\n";
} catch (\ValueError $e) {
    echo "bad_range_rejected: ok\n";
}

echo "ok\n";
?>
--EXPECT--
rects_ge_5: yes
has_line: yes
well_formed_xml: yes
no_target_renders: yes
bad_range_rejected: ok
ok
