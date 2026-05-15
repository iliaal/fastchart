--TEST--
ParetoChart: bars + cumulative line overlay
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

$bars = [
    ['label' => 'A', 'value' => 50],
    ['label' => 'B', 'value' => 30],
    ['label' => 'C', 'value' => 15],
    ['label' => 'D', 'value' => 5],
];

$svg = (new FastChart\ParetoChart(600, 360))
    ->setBars($bars)
    ->renderSvg();

/* 4 filled bar rects + 4 stroke borders + plot rect fill + frame. */
echo "rect_ge_8: ", (substr_count($svg, '<rect') >= 8 ? "yes" : "no"), "\n";
/* Cumulative line: N-1 segments + N circle markers (filled+stroke). */
echo "circles_ge_4: ", (substr_count($svg, '<ellipse') + substr_count($svg, '<circle') >= 4 ? "yes" : "no"), "\n";
echo "lines_present: ", (str_contains($svg, '<line') ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Empty bars throws. */
try {
    (new FastChart\ParetoChart(400, 300))->renderSvg();
    echo "empty_accepted: REGRESSION\n";
} catch (\Throwable $e) {
    echo "empty_rejected: ok\n";
}

echo "ok\n";
?>
--EXPECT--
rect_ge_8: yes
circles_ge_4: yes
lines_present: yes
well_formed_xml: yes
empty_rejected: ok
ok
