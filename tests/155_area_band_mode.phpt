--TEST--
AreaChart::setBandMode() fills envelope between upper and lower series
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Confidence-interval shape: 10 forecast points with ±2.5 envelope. */
$forecast = [10, 12, 11, 14, 13, 16, 15, 18, 17, 20];
$upper = array_map(fn($v) => $v + 2.5, $forecast);
$lower = array_map(fn($v) => $v - 2.5, $forecast);

$c = (new FastChart\AreaChart())
    ->setSize(400, 250)
    ->setSeries([
        ['label' => 'upper', 'data' => $upper],
        ['label' => 'lower', 'data' => $lower],
    ])
    ->setBandMode(true);

$svg = $c->renderSvg();
echo "band has polygon: ", strpos($svg, '<polygon') !== false ? 'yes' : 'NO', "\n";

$png = $c->renderPng();
echo "band png magic: ", substr(bin2hex($png), 0, 8) === '89504e47' ? 'ok' : 'BAD', "\n";

/* Fallback when only one series is supplied — band mode must not
 * crash; behaves like the regular fill-to-baseline. */
$c1 = (new FastChart\AreaChart())
    ->setSize(300, 200)
    ->setSeries([1, 2, 3, 4, 5])
    ->setBandMode(true);
$svg1 = $c1->renderSvg();
echo "fallback single-series: ", strpos($svg1, '<polygon') !== false ? 'ok' : 'BAD', "\n";

/* Stacked + band: stacked wins; band is silently ignored. */
$cs = (new FastChart\AreaChart())
    ->setSize(300, 200)
    ->setSeries([
        ['data' => $upper],
        ['data' => $lower],
    ])
    ->setStacked(true)
    ->setBandMode(true);
$svg_s = $cs->renderSvg();
echo "stacked+band stays stacked: ", strpos($svg_s, '<polygon') !== false ? 'ok' : 'BAD', "\n";
?>
--EXPECT--
band has polygon: yes
band png magic: ok
fallback single-series: ok
stacked+band stays stacked: ok
