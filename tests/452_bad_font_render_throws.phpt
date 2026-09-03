--TEST--
Missing font path throws at setter; unparseable file degrades at render
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php
// Setter fail-fast: a path that does not exist is a user error
// (typo, wrong directory) and throws ValueError up front.
$c = new FastChart\PieChart();
$c->setSlices(['A' => 1, 'B' => 2]);
try {
    $c->setFontPath('/nonexistent/fastchart-no-such-font.ttf');
    echo "NO-THROW\n";
} catch (ValueError $e) {
    echo "setter-throw\n";
}
// An existing but unparseable file passes the setter (existence is
// all it vets) and renders degraded at draw time: the render-time
// fallback contract, not an error.
$c->setFontPath(__FILE__);
$c->setTitle('T');
echo strlen($c->renderSvg()) > 100 ? "degraded-ok\n" : "degraded-empty\n";
// No-font charts still render fine.
$d = new FastChart\PieChart();
$d->setSlices(['A' => 1, 'B' => 2]);
echo strlen($d->renderSvg()) > 100 ? "nofont-ok\n" : "nofont-empty\n";
?>
--EXPECT--
setter-throw
degraded-ok
nofont-ok
