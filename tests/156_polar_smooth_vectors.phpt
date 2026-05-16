--TEST--
PolarChart: INTERP_SMOOTH and addVectors() overlay
--EXTENSIONS--
fastchart
--FILE--
<?php

/* 12-point polar curve with smooth Catmull-Rom interpolation. */
$pts = [];
for ($i = 0; $i < 12; $i++) {
    $a = $i * 30;
    $pts[] = [$a, 5 + 3 * sin($a * M_PI / 180 * 3)];
}

$c = (new FastChart\PolarChart())
    ->setSize(300, 300)
    ->setSeries($pts)
    ->setInterpolation(FastChart\Chart::INTERP_SMOOTH)
    ->setFilled(true);
$svg_smooth = $c->renderSvg();
echo "smooth render: ", strpos($svg_smooth, '<polygon') !== false ? 'ok' : 'BAD', "\n";

$c2 = (new FastChart\PolarChart())
    ->setSize(300, 300)
    ->setSeries($pts)
    ->setInterpolation(FastChart\Chart::INTERP_LINEAR)
    ->setFilled(true);
$svg_linear = $c2->renderSvg();
/* Smooth curve writes more polygon points than the linear one — the
 * polygon's points= attribute is longer. */
preg_match('/<polygon points="([^"]+)"/', $svg_smooth, $m_s);
preg_match('/<polygon points="([^"]+)"/', $svg_linear, $m_l);
$smooth_pts = isset($m_s[1]) ? substr_count($m_s[1], ' ') + 1 : 0;
$linear_pts = isset($m_l[1]) ? substr_count($m_l[1], ' ') + 1 : 0;
echo "smooth points > linear points: ", $smooth_pts > $linear_pts ? 'yes' : 'NO', "\n";

/* Vector overlay: 8 compass directions, radial arrows. */
$vecs = [];
for ($i = 0; $i < 8; $i++) {
    $a = $i * 45;
    $vecs[] = [
        'angle' => $a, 'radius' => 0,
        'angle_to' => $a, 'radius_to' => 3 + $i * 0.3,
    ];
}
$c3 = (new FastChart\PolarChart())
    ->setSize(300, 300)
    ->setSeries([[0, 10]])
    ->setMaxRadius(10)
    ->addVectors($vecs);
$svg_v = $c3->renderSvg();
/* Each vector emits 3 <line> elements (shaft + 2 arrowhead legs). */
echo "vector line count >= 24: ", substr_count($svg_v, '<line') >= 24 ? 'ok' : 'BAD', "\n";

/* Rejecting unknown mode. */
try {
    (new FastChart\PolarChart())->setInterpolation(99);
    echo "BUG: setInterpolation(99) accepted\n";
} catch (ValueError $e) {
    echo "rejected: ", strpos($e->getMessage(), 'INTERP_SMOOTH') !== false ? 'ok' : 'BAD', "\n";
}

/* Missing required vector key. */
try {
    (new FastChart\PolarChart())->addVectors([['angle' => 1, 'radius' => 1]]);
    echo "BUG: addVectors missing keys accepted\n";
} catch (ValueError $e) {
    echo "key check: ", strpos($e->getMessage(), 'angle_to') !== false ? 'ok' : 'BAD', "\n";
}
?>
--EXPECT--
smooth render: ok
smooth points > linear points: yes
vector line count >= 24: ok
rejected: ok
key check: ok
