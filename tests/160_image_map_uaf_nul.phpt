--TEST--
Chart::setImageMap(): reset stale areas + reject embedded NUL
--EXTENSIONS--
fastchart
--FILE--
<?php

/* UAF sequence: setImageMap → render → setImageMap (frees entries
 * the prior areas borrowed) → getImageMap. Without the reset in
 * setImageMap, the second getImageMap reads dangling pointers and
 * ASAN flags heap-use-after-free. With the reset, areas is empty
 * until the next render repopulates it. */
$c = new FastChart\BarChart();
$c->setSize(300, 200)
  ->setSeries([10, 20, 30])
  ->setImageMap([
      ['href' => '/first/1'],
      ['href' => '/first/2'],
      ['href' => '/first/3'],
  ]);
$c->renderSvg();
$map1 = $c->getImageMap('a');
echo "first map has 3 areas: ", substr_count($map1, '<area') === 3 ? 'ok' : 'BAD', "\n";

$c->setImageMap([
    ['href' => '/second/1'],
    ['href' => '/second/2'],
    ['href' => '/second/3'],
]);
/* No render between setImageMap calls — getImageMap should return
 * empty because the old areas were reset and the new entries haven't
 * been rendered yet. Critical: must not UAF read into freed entries. */
$map2 = $c->getImageMap('b');
echo "post-reset map empty: ", $map2 === '' ? 'ok' : 'BAD', "\n";

$c->renderSvg();
$map3 = $c->getImageMap('c');
echo "second render has /second/2: ", strpos($map3, 'href="/second/2"') !== false ? 'ok' : 'BAD', "\n";

/* Embedded NUL rejection: `/safe\0javascript:alert(1)` would
 * otherwise pass the visible-prefix scheme allowlist while the
 * downstream copy holds the full PHP string. setImageMap should
 * drop the entry's href (and tooltip) silently. */
$c2 = new FastChart\PieChart();
$c2->setSize(200, 200)
   ->setSlices(['x' => 1, 'y' => 2])
   ->setImageMap([
       ['href' => "/safe\0javascript:alert(1)", 'tooltip' => "tip\0evil"],
       ['href' => '/clean', 'tooltip' => 'plain'],
   ]);
$c2->renderSvg();
$pie_map = $c2->getImageMap();
echo "nul-poisoned href dropped: ", strpos($pie_map, 'javascript:') === false ? 'ok' : 'BAD', "\n";
echo "nul-poisoned tooltip dropped: ", strpos($pie_map, 'tip') === false ? 'ok' : 'BAD', "\n";
echo "clean entry kept: ", strpos($pie_map, 'href="/clean"') !== false ? 'ok' : 'BAD', "\n";
?>
--EXPECT--
first map has 3 areas: ok
post-reset map empty: ok
second render has /second/2: ok
nul-poisoned href dropped: ok
nul-poisoned tooltip dropped: ok
clean entry kept: ok
