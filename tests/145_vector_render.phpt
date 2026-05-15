--TEST--
VectorChart: arrow-on-grid vector field
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* 4x4 grid of vectors swirling around the origin. */
$vecs = [];
for ($x = 0; $x < 4; $x++) {
    for ($y = 0; $y < 4; $y++) {
        $vecs[] = ['x' => $x, 'y' => $y,
                   'dx' => -($y - 1.5) * 0.3,
                   'dy' =>  ($x - 1.5) * 0.3];
    }
}

$svg = (new FastChart\VectorChart(500, 500))
    ->setVectors($vecs)
    ->setColorRamp(0xDDDDFF, 0x0000AA)
    ->renderSvg();

/* Each arrow = 1 line + 1 triangle polygon. 16 arrows. */
echo "lines_ge_16: ", (substr_count($svg, '<line') >= 16 ? "yes" : "no"), "\n";
echo "polygons_ge_16: ", (substr_count($svg, '<polygon') >= 16 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Empty vectors throws. */
try {
    (new FastChart\VectorChart(400, 400))->renderSvg();
    echo "empty_accepted: REGRESSION\n";
} catch (\Throwable $e) {
    echo "empty_rejected: ok\n";
}

/* Non-finite components silently dropped. */
$svg2 = (new FastChart\VectorChart(400, 400))
    ->setVectors([
        ['x' => 0, 'y' => 0, 'dx' => INF, 'dy' => 0],
        ['x' => 0, 'y' => 1, 'dx' => 1,   'dy' => 1],
    ])
    ->renderSvg();
echo "bad_vec_dropped: ", (strlen($svg2) > 100 ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
lines_ge_16: yes
polygons_ge_16: yes
well_formed_xml: yes
empty_rejected: ok
bad_vec_dropped: yes
ok
