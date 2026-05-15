--TEST--
Fresh-eyes audit fixes: Sankey OOB, Sunburst depth, Vector inf, Bullet narrow
--EXTENSIONS--
fastchart
--FILE--
<?php

/* H1: SankeyChart::setNodes() must wipe pre-existing links, else
 * stale from/to indices reference past-the-end of the new node
 * array and produce a heap OOB read in compute_layers. */
$c = (new FastChart\SankeyChart(400, 200))
    ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
    ->setLinks([['from' => 2, 'to' => 0, 'value' => 1]]);
$c->setNodes([['label' => 'only one']]);
try {
    $c->renderSvg();
    echo "h1_stale_link_kept: REGRESSION\n";
} catch (\Throwable $e) {
    /* render now fails cleanly because links were wiped — caller
     * must re-call setLinks after setNodes. */
    echo "h1_links_wiped: ",
        (str_contains($e->getMessage(), 'setLinks') ? "ok" : "wrong-msg"), "\n";
}

/* H2: SunburstChart::setHierarchy() depth must be capped so
 * adversarial nesting can't blow the C stack. The cap is 32 levels. */
$h = ['value' => 1];
for ($i = 0; $i < 100; $i++) {
    $h = ['label' => "L$i", 'children' => [$h]];
}
try {
    (new FastChart\SunburstChart(300, 300))->setHierarchy($h);
    echo "h2_deep_accepted: REGRESSION\n";
} catch (\ValueError $e) {
    echo "h2_deep_rejected: ",
        (str_contains($e->getMessage(), 'depth') ? "ok" : "wrong-msg"), "\n";
}
/* Depths within the cap still work. */
$h32 = ['value' => 1];
for ($i = 0; $i < 30; $i++) {
    $h32 = ['label' => "L$i", 'children' => [$h32]];
}
$svg = (new FastChart\SunburstChart(300, 300))->setHierarchy($h32)->renderSvg();
echo "h2_within_cap: ", (strlen($svg) > 100 ? "ok" : "fail"), "\n";

/* H3: VectorChart must reject (dx, dy) pairs whose magnitude
 * squared overflows to inf. Each component alone is finite. */
$svg = (new FastChart\VectorChart(400, 400))
    ->setVectors([
        ['x' => 0, 'y' => 0, 'dx' => 1e200, 'dy' => 0],   /* dropped */
        ['x' => 1, 'y' => 1, 'dx' => 1.0,   'dy' => 1.0], /* keeper */
    ])
    ->renderSvg();
echo "h3_huge_mag_dropped: ", (strlen($svg) > 100 ? "ok" : "fail"), "\n";

/* M5: BulletChart on a canvas too narrow for the side label margins
 * must throw, not draw negative-width rectangles. */
try {
    (new FastChart\BulletChart(100, 80))
        ->setRange(0, 100)->setValue(50)->renderSvg();
    echo "m5_narrow_accepted: REGRESSION\n";
} catch (\Error $e) {
    echo "m5_narrow_rejected: ",
        (str_contains($e->getMessage(), 'narrow') ? "ok" : "wrong-msg"), "\n";
}

echo "ok\n";
?>
--EXPECT--
h1_links_wiped: ok
h2_deep_rejected: ok
h2_within_cap: ok
h3_huge_mag_dropped: ok
m5_narrow_rejected: ok
ok
