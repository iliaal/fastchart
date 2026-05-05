--TEST--
ScatterChart::getImageMap returns clickable HTML areas for points with href
--EXTENSIONS--
fastchart
--FILE--
<?php

$c = new FastChart\ScatterChart(500, 400);
$c->setPoints([
    [1.0, 2.0, 'href' => '/q1', 'tooltip' => 'Q1 sales'],
    [2.0, 3.0, 'href' => '/q2'],
    [3.0, 1.5],   // no href -> not in map
]);
$bytes = $c->renderPng();
echo "renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

$map = $c->getImageMap('sales');
echo "name: ",       (str_contains($map, 'name="sales"')    ? "yes" : "no"), "\n";
echo "q1_href: ",    (str_contains($map, 'href="/q1"')       ? "yes" : "no"), "\n";
echo "q1_tooltip: ", (str_contains($map, 'title="Q1 sales"') ? "yes" : "no"), "\n";
echo "q2_href: ",    (str_contains($map, 'href="/q2"')       ? "yes" : "no"), "\n";
echo "no_href_count: ", substr_count($map, '<area'), "\n";

// Without any href, returns the empty string.
$c2 = (new FastChart\ScatterChart(400, 300))
    ->setPoints([[1, 2], [3, 4]]);
$c2->renderPng();
$map2 = $c2->getImageMap();
echo "empty_map: ", $map2 === '' ? "yes" : "no ($map2)", "\n";
?>
--EXPECT--
renders: ok
name: yes
q1_href: yes
q1_tooltip: yes
q2_href: yes
no_href_count: 2
empty_map: yes
