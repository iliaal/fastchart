--TEST--
SerpentineTimeline: boustrophedon event path
--EXTENSIONS--
fastchart
simplexml
gd
--FILE--
<?php

$events = [];
foreach (['Plan', 'Design', 'Build', 'Test', 'Ship', 'Measure', 'Iterate', 'Scale', 'Support', 'Sunset'] as $k => $lbl) {
    $events[] = ['label' => $lbl, 'date' => 'Q' . (($k % 4) + 1),
                 'color' => ($k % 2 ? 0x4488cc : 0xcc8844)];
}
$svg = (new FastChart\SerpentineTimeline(700, 400))
    ->setEvents($events)
    ->setColumns(4)
    ->renderSvg();

/* 10 markers (fill + stroke = 2 each) + 9 connecting segments. */
echo "circles_eq_20: ", (substr_count($svg, '<circle') === 20 ? "yes" : "no"), "\n";
echo "lines_eq_9: ", (substr_count($svg, '<line') === 9 ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Filled marker circles are emitted in event order. Verify the snake:
 * row 0 (events 0..3) runs left->right, row 1 (events 4..7) runs back
 * right->left, so event 3 and event 4 share roughly the same x (the
 * U-turn) and event 4 sits on a lower row. */
preg_match_all('/<circle cx="([-\d.]+)" cy="([-\d.]+)" r="[\d.]+" fill="#/',
               $svg, $m, PREG_SET_ORDER);
echo "markers_eq_10: ", (count($m) === 10 ? "yes" : "no"), "\n";

$x = array_map(fn($c) => (float)$c[1], $m);
$y = array_map(fn($c) => (float)$c[2], $m);

/* Row 0 ascending x. */
echo "row0_l2r: ", ($x[0] < $x[1] && $x[1] < $x[2] && $x[2] < $x[3] ? "yes" : "no"), "\n";
/* Row 1 descending x (reversed). */
echo "row1_r2l: ", ($x[4] > $x[5] && $x[5] > $x[6] && $x[6] > $x[7] ? "yes" : "no"), "\n";
/* U-turn: event 4 roughly under event 3, on a lower row. */
echo "uturn: ", (abs($x[3] - $x[4]) < 5 && $y[4] > $y[3] ? "yes" : "no"), "\n";

/* No events is an error. */
try {
    (new FastChart\SerpentineTimeline(300, 200))->renderSvg();
    echo "empty: no_throw\n";
} catch (\Throwable $e) {
    echo "empty: threw\n";
}

/* Raster round-trip. */
$im = imagecreatefromstring(
    (new FastChart\SerpentineTimeline(320, 200))
        ->setEvents([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->renderPng());
echo "png_ok: ", ($im !== false ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
circles_eq_20: yes
lines_eq_9: yes
well_formed_xml: yes
markers_eq_10: yes
row0_l2r: yes
row1_r2l: yes
uturn: yes
empty: threw
png_ok: yes
ok
