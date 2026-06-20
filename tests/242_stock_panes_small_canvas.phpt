--TEST--
StockChart: many indicator panes on a short canvas keep the price pane from inverting
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_7d6d5c16: the 24px pane-height floor could exceed the 60% cap on a short
 * canvas, pushing the price pane to negative height and panes above plot.y0.
 * The pane height is now re-capped so the price pane never inverts. */

use FastChart\StockChart;

$rows = [];
for ($i = 0; $i < 8; $i++) {
    $o = 100 + $i;
    $rows[] = [1700000000 + $i * 86400, $o, $o + 2, $o - 2, $o + 1, 1000];
}
$c = (new StockChart())->setSize(320, 120)->setOhlcv($rows);
for ($p = 0; $p < 6; $p++) {
    $c->addIndicatorPane("ind$p", array_fill(0, 8, $p + 1));
}
$svg = $c->renderSvg();

echo "renders: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";
echo "no_negative_coords: ", (preg_match('/="-\d/', $svg) ? 'no' : 'yes'), "\n";

?>
--EXPECT--
renders: yes
no_negative_coords: yes
