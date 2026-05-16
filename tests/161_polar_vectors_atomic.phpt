--TEST--
PolarChart::addVectors(): atomic commit — partial entries don't persist on throw
--EXTENSIONS--
fastchart
--FILE--
<?php

/* First valid call lands cleanly. */
$c = (new FastChart\PolarChart())
    ->setSize(200, 200)
    ->setSeries([[0, 5]])
    ->setMaxRadius(10);
$c->addVectors([
    ['angle' => 0,  'radius' => 0, 'angle_to' => 0,  'radius_to' => 3],
    ['angle' => 90, 'radius' => 0, 'angle_to' => 90, 'radius_to' => 4],
]);
echo "first add: ok\n";

/* Mid-list bad entry: missing radius_to. The first entry in this
 * batch is valid, but the second is malformed. Without atomic
 * commit, the first entry would leak into self->vectors and a
 * retry would duplicate it. */
$pre_svg = $c->renderSvg();
$pre_lines = substr_count($pre_svg, '<line');
try {
    $c->addVectors([
        ['angle' => 180, 'radius' => 0, 'angle_to' => 180, 'radius_to' => 5],
        ['angle' => 270, 'radius' => 0, 'angle_to' => 270],   /* bad */
    ]);
    echo "BUG: missing radius_to accepted\n";
} catch (\ValueError $e) {
    echo "rejected: ", strpos($e->getMessage(), 'radius_to') !== false ? 'ok' : 'BAD', "\n";
}
$post_svg = $c->renderSvg();
$post_lines = substr_count($post_svg, '<line');
echo "no partial commit: ", $pre_lines === $post_lines ? 'ok' : "BAD ($pre_lines vs $post_lines)", "\n";

/* Non-array entry: first valid, second wrong type. Same atomicity. */
try {
    $c->addVectors([
        ['angle' => 45, 'radius' => 0, 'angle_to' => 45, 'radius_to' => 6],
        'oops',
    ]);
    echo "BUG: non-array accepted\n";
} catch (\TypeError $e) {
    echo "rejected: ", strpos($e->getMessage(), 'array') !== false ? 'ok' : 'BAD', "\n";
}
$after_svg = $c->renderSvg();
echo "still no partial commit: ", substr_count($after_svg, '<line') === $pre_lines ? 'ok' : 'BAD', "\n";

/* Valid retry works and DOESN'T duplicate the rejected entries. */
$c->addVectors([
    ['angle' => 180, 'radius' => 0, 'angle_to' => 180, 'radius_to' => 5],
    ['angle' => 270, 'radius' => 0, 'angle_to' => 270, 'radius_to' => 6],
]);
$final_svg = $c->renderSvg();
/* 2 initial + 2 retry = 4 vectors × 3 line elements (shaft + 2
 * arrowhead legs) = 12 line elements at minimum. */
echo "retry adds cleanly: ", substr_count($final_svg, '<line') >= $pre_lines + 6 ? 'ok' : 'BAD', "\n";
?>
--EXPECT--
first add: ok
rejected: ok
no partial commit: ok
rejected: ok
still no partial commit: ok
retry adds cleanly: ok
