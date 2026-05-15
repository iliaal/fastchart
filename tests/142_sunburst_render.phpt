--TEST--
SunburstChart: nested ring polygons
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

$svg = (new FastChart\SunburstChart(400, 400))
    ->setHierarchy([
        'label' => 'root',
        'children' => [
            ['label' => 'A', 'children' => [
                ['label' => 'A1', 'value' => 10],
                ['label' => 'A2', 'value' => 20],
            ]],
            ['label' => 'B', 'value' => 30],
            ['label' => 'C', 'children' => [
                ['label' => 'C1', 'value' => 5],
                ['label' => 'C2', 'value' => 5],
                ['label' => 'C3', 'value' => 5],
            ]],
        ],
    ])
    ->renderSvg();

/* 3 top-level + 5 leaf wedges = 8 polygons × 2 (fill + stroke). */
echo "polygons_ge_8: ", (substr_count($svg, '<polygon') >= 8 ? "yes" : "no"), "\n";
echo "has_root_circle: ",
    (str_contains($svg, '<circle') || str_contains($svg, '<ellipse')
        ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* All-zero hierarchy throws. */
try {
    (new FastChart\SunburstChart(300, 300))
        ->setHierarchy(['children' => [['value' => 0]]])
        ->renderSvg();
    echo "zero_accepted: REGRESSION\n";
} catch (\Throwable $e) {
    echo "zero_rejected: ok\n";
}

echo "ok\n";
?>
--EXPECT--
polygons_ge_8: yes
has_root_circle: yes
well_formed_xml: yes
zero_rejected: ok
ok
