--TEST--
Round-4 audit fixes: Sunburst atomic, <use> rejection
--EXTENSIONS--
fastchart
--FILE--
<?php

/* A1: SunburstChart::setHierarchy() must be atomic on depth overflow.
 * Pre-fix, the setter released self->nodes/count/depth BEFORE calling
 * build_rec; a depth > 32 input threw \ValueError and the chart was
 * left with an empty hierarchy. */
$good = [
    'label' => 'root',
    'children' => [
        ['label' => 'a', 'value' => 10],
        ['label' => 'b', 'value' => 20],
        ['label' => 'c', 'value' => 30],
    ],
];
$c = (new FastChart\SunburstChart(400, 400))->setHierarchy($good);
$before = $c->renderSvg();

/* Build a 100-level-deep tree to exceed the depth=32 cap. */
$bad = ['value' => 1];
for ($i = 0; $i < 100; $i++) {
    $bad = ['label' => "L$i", 'children' => [$bad]];
}
try {
    $c->setHierarchy($bad);
    echo "depth_overflow_accepted: REGRESSION\n";
} catch (\ValueError $e) {
    $after = $c->renderSvg();
    echo "sunburst_atomic: ", ($before === $after ? "ok" : "fail"), "\n";
}

/* Re-set with another valid hierarchy must still work post-throw. */
$c->setHierarchy([
    'label' => 'new',
    'children' => [['label' => 'x', 'value' => 5]],
]);
echo "sunburst_recoverable: ",
    ($c->renderSvg() !== $before ? "ok" : "fail"), "\n";

/* A2: Chart::svgToPng/Jpeg/Webp() reject ANY <use> element. plutosvg's
 * <use href="#id"> renderer walks the referenced subtree inline; the
 * cycle detector compares element pointers along the ancestor chain
 * but does NOT count fan-out. A 1.4 KB SVG with 8 nested <g> levels
 * where each contains 10x <use> of the next triggers ~10^8 shape
 * renders (~14s on commodity hardware) — billion-laughs equivalent
 * that escapes any naive source-count cap.
 *
 * A prior fix capped source <use> count at 256; that was insufficient
 * because nesting amplifies independently of source count. Reject
 * any <use> at the entry point. */
$benign = '<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">'
        . '<rect width="100" height="100" fill="#FF0000"/></svg>';
$png = FastChart\Chart::svgToPng($benign);
echo "no_use_ok: ", (substr($png, 0, 8) === "\x89PNG\r\n\x1A\n" ? "ok" : "fail"), "\n";

/* Single <use> — would have been accepted under the old count cap,
 * now rejected. */
$one_use = '<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">'
         . '<defs><g id="a"><rect width="10" height="10"/></g></defs>'
         . '<use href="#a"/></svg>';
foreach (['svgToPng', 'svgToJpeg', 'svgToWebp'] as $m) {
    try {
        FastChart\Chart::$m($one_use);
        echo "use_accepted_$m: REGRESSION\n";
    } catch (\ValueError $e) {
        echo "use_rejected_$m: ",
            (str_contains($e->getMessage(), '<use>') ? "ok" : "wrong-msg"), "\n";
    }
}

/* The nested fan-out attack the count cap missed: only 71 source
 * <use> tags but exponential expansion. Must also reject. */
$fan_svg = '<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100"><defs>';
$fan_svg .= '<g id="a"><rect width="1" height="1"/></g>';
for ($i = 1; $i < 8; $i++) {
    $prev = $i - 1 === 0 ? 'a' : 'g' . ($i - 1);
    $cur  = "g$i";
    $body = str_repeat("<use href=\"#$prev\"/>", 10);
    $fan_svg .= "<g id=\"$cur\">$body</g>";
}
$fan_svg .= '</defs><use href="#g7"/></svg>';
try {
    FastChart\Chart::svgToPng($fan_svg);
    echo "fanout_accepted: REGRESSION\n";
} catch (\ValueError $e) {
    echo "fanout_rejected: ok\n";
}

/* Boundary check: <userdata> must NOT trigger rejection. */
$mixed = '<svg xmlns="http://www.w3.org/2000/svg" width="50" height="50">'
       . '<!-- <usefulinfo>blah</usefulinfo> -->'
       . '<rect width="50" height="50" fill="#00FF00"/></svg>';
$mixed_png = FastChart\Chart::svgToPng($mixed);
echo "boundary_narrow_match: ",
    (substr($mixed_png, 0, 8) === "\x89PNG\r\n\x1A\n" ? "ok" : "fail"), "\n";

/* Uppercase <USE > also rejected (defense-in-depth even though
 * plutosvg's tag lookup is lowercase). */
$upper_svg = '<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">'
           . '<defs><g id="a"><rect/></g></defs>'
           . '<USE href="#a"/></svg>';
try {
    FastChart\Chart::svgToPng($upper_svg);
    echo "upper_use_accepted: REGRESSION\n";
} catch (\ValueError $e) {
    echo "upper_use_rejected: ok\n";
}

echo "ok\n";
?>
--EXPECT--
sunburst_atomic: ok
sunburst_recoverable: ok
no_use_ok: ok
use_rejected_svgToPng: ok
use_rejected_svgToJpeg: ok
use_rejected_svgToWebp: ok
fanout_rejected: ok
boundary_narrow_match: ok
upper_use_rejected: ok
ok
