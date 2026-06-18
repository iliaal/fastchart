--TEST--
NetworkChart: deterministic force-directed layout
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

function net(int $seed, int $iters = 200): FastChart\NetworkChart {
    return (new FastChart\NetworkChart(500, 400))
        ->setNodes([
            ['label' => 'A'], ['label' => 'B'], ['label' => 'C'],
            ['label' => 'D'], ['label' => 'E'], ['label' => 'F'],
        ])
        ->setLinks([
            ['from' => 0, 'to' => 1, 'value' => 2],
            ['from' => 1, 'to' => 2, 'value' => 1],
            ['from' => 2, 'to' => 0, 'value' => 3],
            ['from' => 3, 'to' => 4, 'value' => 1],
            ['from' => 4, 'to' => 5, 'value' => 2],
            ['from' => 0, 'to' => 3, 'value' => 1],
        ])
        ->setSeed($seed)
        ->setIterations($iters);
}

$a = net(1)->renderSvg();
$b = net(1)->renderSvg();
$c = net(7)->renderSvg();

/* Same input + same seed must be byte-identical (no Math.random). */
echo "deterministic: ", ($a === $b ? "yes" : "no"), "\n";
/* A different seed must change the layout. */
echo "seed_sensitive: ", ($a !== $c ? "yes" : "no"), "\n";

/* 6 edges (lines) and 6 node markers (filled circle + stroke = 12). */
echo "lines_eq_6: ", (substr_count($a, '<line') === 6 ? "yes" : "no"), "\n";
echo "circles_eq_12: ", (substr_count($a, '<circle') === 12 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($a, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Bad links dropped. */
$svg2 = (new FastChart\NetworkChart(300, 300))
    ->setNodes([['label' => 'A'], ['label' => 'B']])
    ->setLinks([
        ['from' => 0, 'to' => 0, 'value' => 1],   /* self-loop */
        ['from' => 0, 'to' => 9, 'value' => 1],   /* out of range */
        ['from' => 0, 'to' => 1, 'value' => 2],   /* keeper */
    ])
    ->renderSvg();
echo "bad_links_dropped: ", (substr_count($svg2, '<line') === 1 ? "yes" : "no"), "\n";

/* No nodes / links is an error. */
try {
    (new FastChart\NetworkChart(300, 300))->renderSvg();
    echo "empty: no_throw\n";
} catch (\Throwable $e) {
    echo "empty: threw\n";
}

/* Raster round-trip emits a valid PNG. */
$im = imagecreatefromstring(net(1)->renderPng());
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";
if ($im) { imagedestroy($im); }

echo "ok\n";
?>
--EXPECT--
deterministic: yes
seed_sensitive: yes
lines_eq_6: yes
circles_eq_12: yes
well_formed_xml: yes
bad_links_dropped: yes
empty: threw
png_ok: yes
ok
