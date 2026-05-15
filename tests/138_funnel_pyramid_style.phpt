--TEST--
Funnel::setStyle(STYLE_PYRAMID): subdivides into value-proportional bands
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Funnel and Pyramid share the same stage data but lay it out
 * differently:
 *  - FUNNEL (default): equal-height trapezoids; widths track each
 *    stage's value relative to the largest stage.
 *  - PYRAMID: single triangle subdivided into bands; band heights
 *    are proportional to stage values (cumulative within the
 *    triangle); widths follow the triangle's natural taper. */

$stages = [
    ['label' => 'Visit',   'value' => 12400],
    ['label' => 'Sign up', 'value' =>  3200],
    ['label' => 'Active',  'value' =>  1950],
    ['label' => 'Trial',   'value' =>   820],
    ['label' => 'Paid',    'value' =>   310],
];

$f = (new FastChart\Funnel(400, 300))->setStages($stages)->renderSvg();
$p = (new FastChart\Funnel(400, 300))
    ->setStages($stages)
    ->setStyle(FastChart\Funnel::STYLE_PYRAMID)
    ->renderSvg();

/* Both render five filled trapezoids + five edge outlines = 10
 * polygons each, but the polygon geometry differs. */
echo "funnel_polys: ",  substr_count($f, '<polygon'), "\n";
echo "pyramid_polys: ", substr_count($p, '<polygon'), "\n";
echo "layouts_differ: ", ($f === $p ? "no" : "yes"), "\n";

/* Setter validation. */
try {
    (new FastChart\Funnel(400, 300))->setStages($stages)->setStyle(7);
    echo "bad_style_accepted: REGRESSION\n";
} catch (\ValueError $e) {
    $msg = $e->getMessage();
    echo "bad_style_rejected: ",
        (str_contains($msg, 'STYLE_FUNNEL') && str_contains($msg, 'STYLE_PYRAMID')
            ? "ok" : "wrong-msg"), "\n";
}

/* Class constants exposed at the documented values. */
echo "STYLE_FUNNEL: ",  FastChart\Funnel::STYLE_FUNNEL,  "\n";
echo "STYLE_PYRAMID: ", FastChart\Funnel::STYLE_PYRAMID, "\n";

echo "ok\n";
?>
--EXPECT--
funnel_polys: 10
pyramid_polys: 10
layouts_differ: yes
bad_style_rejected: ok
STYLE_FUNNEL: 0
STYLE_PYRAMID: 1
ok
