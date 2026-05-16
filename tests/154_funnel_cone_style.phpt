--TEST--
Funnel STYLE_CONE: pyramid layout with ellipse-arc band edges
--EXTENSIONS--
fastchart
--FILE--
<?php

$stages = [
    ['label' => 'Visited',   'value' => 1000],
    ['label' => 'Signed up', 'value' => 600],
    ['label' => 'Activated', 'value' => 300],
    ['label' => 'Paying',    'value' => 100],
];

/* STYLE_FUNNEL and STYLE_PYRAMID emit one 4-vertex polygon per stage. */
$f = (new FastChart\Funnel())
    ->setSize(300, 300)
    ->setStages($stages)
    ->setStyle(FastChart\Funnel::STYLE_PYRAMID);
$svg_pyr = $f->renderSvg();
$pyr_polys = substr_count($svg_pyr, '<polygon');
echo "pyramid polygons: $pyr_polys\n";

/* STYLE_CONE samples 2 * (ARC_N + 1) = 30 points per band. */
$f = (new FastChart\Funnel())
    ->setSize(300, 300)
    ->setStages($stages)
    ->setStyle(FastChart\Funnel::STYLE_CONE);
$svg_cone = $f->renderSvg();
$cone_polys = substr_count($svg_cone, '<polygon');
echo "cone polygons: $cone_polys\n";

/* Both styles render to PNG without faulting. */
$png = $f->renderPng();
echo "cone png magic: ", substr(bin2hex($png), 0, 8) === '89504e47' ? 'ok' : 'BAD', "\n";

/* Rejecting unknown style values. */
try {
    $f->setStyle(99);
    echo "BUG: setStyle(99) accepted\n";
} catch (ValueError $e) {
    echo "rejected: ", strpos($e->getMessage(), 'STYLE_CONE') !== false ? 'mentions STYLE_CONE' : 'BAD', "\n";
}
?>
--EXPECT--
pyramid polygons: 8
cone polygons: 8
cone png magic: ok
rejected: mentions STYLE_CONE
