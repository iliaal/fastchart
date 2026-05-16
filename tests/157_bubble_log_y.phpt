--TEST--
BubbleChart: setYAxisScale(SCALE_LOG) maps exponential data onto log axis
--EXTENSIONS--
fastchart
--FILE--
<?php

/* 20 bubbles whose Y values span 4 orders of magnitude. */
$pts = [];
for ($i = 1; $i <= 20; $i++) {
    $pts[] = [$i, pow(2, $i / 2), 10 + $i];
}

$c = (new FastChart\BubbleChart())
    ->setSize(400, 250)
    ->setPoints($pts)
    ->setYAxisScale(FastChart\Chart::SCALE_LOG);

$svg = $c->renderSvg();
echo "log axis renders: ", strpos($svg, '<circle') !== false ? 'ok' : 'BAD', "\n";

$png = $c->renderPng();
echo "log axis png ok: ", substr(bin2hex($png), 0, 8) === '89504e47' ? 'ok' : 'BAD', "\n";

/* Log scale with non-positive data must throw. */
try {
    (new FastChart\BubbleChart())
        ->setSize(300, 200)
        ->setPoints([[1, -2, 5], [2, 3, 5]])
        ->setYAxisScale(FastChart\Chart::SCALE_LOG)
        ->renderSvg();
    echo "BUG: negative Y accepted\n";
} catch (ValueError $e) {
    echo "rejects negative Y: ", strpos($e->getMessage(), 'positive') !== false ? 'ok' : 'BAD', "\n";
}

/* Default linear scale still works on the same data after toggling. */
$c2 = (new FastChart\BubbleChart())
    ->setSize(400, 250)
    ->setPoints($pts)
    ->setYAxisScale(FastChart\Chart::SCALE_LINEAR);
echo "linear default still works: ", strpos($c2->renderSvg(), '<circle') !== false ? 'ok' : 'BAD', "\n";
?>
--EXPECT--
log axis renders: ok
log axis png ok: ok
rejects negative Y: ok
linear default still works: ok
