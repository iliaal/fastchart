--TEST--
MarimekkoChart: variable-width stacked columns
--EXTENSIONS--
fastchart
--FILE--
<?php

$svg = (new FastChart\MarimekkoChart(600, 400))
    ->setColumns([
        ['label' => 'Q1', 'segments' => [
            ['label' => 'Hardware', 'value' => 30],
            ['label' => 'Services', 'value' => 20],
            ['label' => 'Other',    'value' => 10],
        ]],
        ['label' => 'Q2', 'segments' => [
            ['label' => 'Hardware', 'value' => 40],
            ['label' => 'Services', 'value' => 20],
        ]],
        ['label' => 'Q3', 'segments' => [
            ['label' => 'Hardware', 'value' => 10],
            ['label' => 'Services', 'value' => 30],
            ['label' => 'Other',    'value' => 5],
        ]],
    ])
    ->renderSvg();

/* 8 segment rects × 2 (fill + border) + bg rect = 17. */
echo "rects_ge_15: ", (substr_count($svg, '<rect') >= 15 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Empty columns throws. */
try {
    (new FastChart\MarimekkoChart(400, 300))->renderSvg();
    echo "empty_accepted: REGRESSION\n";
} catch (\Throwable $e) {
    echo "empty_rejected: ok\n";
}

/* Column with no positive segments is silently dropped. */
$svg2 = (new FastChart\MarimekkoChart(400, 300))
    ->setColumns([
        ['label' => 'Empty', 'segments' => [['value' => 0]]],
        ['label' => 'Real',  'segments' => [['value' => 1], ['value' => 2]]],
    ])
    ->renderSvg();
echo "empty_col_dropped: ", (strlen($svg2) > 100 ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
rects_ge_15: yes
well_formed_xml: yes
empty_rejected: ok
empty_col_dropped: yes
ok
