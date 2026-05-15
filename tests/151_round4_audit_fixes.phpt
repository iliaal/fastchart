--TEST--
Round-4 audit fixes: Sunburst atomic, <use> fan-out cap
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

/* A2: Chart::svgToPng/Jpeg/Webp() must reject SVG with > 256
 * <use> elements. plutosvg's <use href="#id"> renders the
 * referenced subtree inline; the cycle detector compares element
 * pointers along the ancestor chain but does NOT count fan-out.
 * A 2 KB SVG with 10 nested <g> levels, each containing 10x <use>
 * of the next, can trigger 10^10 shape renders (billion-laughs
 * equivalent). */
$ok = '<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">'
    . '<defs><g id="a"><rect width="10" height="10"/></g></defs>'
    . '<use href="#a"/></svg>';
$png = FastChart\Chart::svgToPng($ok);
echo "one_use_ok: ", (substr($png, 0, 8) === "\x89PNG\r\n\x1A\n" ? "ok" : "fail"), "\n";

/* 257 <use> elements (one over the cap). */
$many = str_repeat('<use href="#a"/>', 257);
$bad_svg = '<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">'
    . '<defs><g id="a"><rect width="10" height="10"/></g></defs>'
    . $many . '</svg>';
foreach (['svgToPng', 'svgToJpeg', 'svgToWebp'] as $m) {
    try {
        FastChart\Chart::$m($bad_svg);
        echo "fan_out_accepted_$m: REGRESSION\n";
    } catch (\ValueError $e) {
        echo "fan_out_rejected_$m: ",
            (str_contains($e->getMessage(), '<use>') ? "ok" : "wrong-msg"), "\n";
    }
}

/* Boundary check: must NOT match <userdata or <usepath or <UseRange.
 * Tag-name boundary check (whitespace / > / /) keeps the pattern
 * narrow. The example below uses both <use> (1 instance) and an
 * unrelated <userdata>-like construct (in a comment, so plutosvg
 * skips it; the scanner has to also skip it). */
$mixed = '<svg xmlns="http://www.w3.org/2000/svg" width="50" height="50">'
       . '<!-- <usefulinfo>blah</usefulinfo> -->'
       . '<defs><g id="a"><rect width="5" height="5"/></g></defs>'
       . '<use href="#a"/></svg>';
$mixed_png = FastChart\Chart::svgToPng($mixed);
echo "boundary_narrow_match: ",
    (substr($mixed_png, 0, 8) === "\x89PNG\r\n\x1A\n" ? "ok" : "fail"), "\n";

/* Uppercase <USE > should also count toward the cap (defense-in-
 * depth even though plutosvg's tag lookup is lowercase). */
$upper_uses = str_repeat('<USE href="#a"/>', 300);
$upper_svg = '<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">'
           . '<defs><g id="a"><rect/></g></defs>'
           . $upper_uses . '</svg>';
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
one_use_ok: ok
fan_out_rejected_svgToPng: ok
fan_out_rejected_svgToJpeg: ok
fan_out_rejected_svgToWebp: ok
boundary_narrow_match: ok
upper_use_rejected: ok
ok
