--TEST--
Rendered-text setters cap at 8192 bytes; array labels drop silently
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Glyph-path SVG output replays each glyph inline, amplifying text
 * ~200x, so a single unbounded string balloons the document. Scalar
 * text setters throw above 8192 bytes; oversized array-element labels
 * drop silently (matching the label helper's embedded-NUL contract). */

// Scalar setter: 8192 accepted and rendered, 8193 rejected at set time.
$ok = (new FastChart\LineChart(400, 300))
    ->setSeries([1, 2, 3])
    ->setTitle(str_repeat('A', 8192));
echo "title 8192 renders: ", strlen($ok->renderSvg()) > 0 ? "ok" : "FAIL", "\n";

try {
    (new FastChart\LineChart(400, 300))->setTitle(str_repeat('A', 8193));
    echo "title 8193: NO THROW\n";
} catch (\ValueError $e) {
    echo "title 8193: ",
        $e->getMessage() === 'FastChart\Chart::setTitle() text exceeds the 8192-byte limit'
        ? "ok" : $e->getMessage(), "\n";
}

// Axis title and annotation share the cap.
try {
    (new FastChart\LineChart(400, 300))->setXAxisTitle(str_repeat('X', 8193));
    echo "xaxis 8193: NO THROW\n";
} catch (\ValueError $e) {
    echo "xaxis 8193: ",
        $e->getMessage() === 'FastChart\Chart::setXAxisTitle() text exceeds the 8192-byte limit'
        ? "ok" : $e->getMessage(), "\n";
}

// Array-element label over the cap is dropped, not thrown: the render
// succeeds and the 9000-byte run never reaches the document.
$big = str_repeat('L', 9000);
$svg = (new FastChart\LineChart(400, 300))
    ->setSeries([['label' => $big, 'data' => [1, 2, 3]]])
    ->renderSvg();
echo "9000-byte series label dropped: ",
    (strlen($svg) > 0 && !str_contains($svg, $big)) ? "ok" : "FAIL", "\n";

// Treemap / Funnel / Waterfall parse labels manually (not through the
// shared label helper) — each must apply the same cap. A dropped label
// renders byte-identically to the same chart with no label at all.
$huge = str_repeat('A', 100000);

$with = (new FastChart\Funnel(300, 200))
    ->setStages([['value' => 10, 'label' => $huge], ['value' => 5]])
    ->renderSvg();
$without = (new FastChart\Funnel(300, 200))
    ->setStages([['value' => 10], ['value' => 5]])
    ->renderSvg();
echo "funnel 100k stage label dropped: ", $with === $without ? "ok" : "FAIL", "\n";

$with = (new FastChart\Waterfall(300, 200))
    ->setBars([['value' => 10, 'label' => $huge], ['value' => 5]])
    ->renderSvg();
$without = (new FastChart\Waterfall(300, 200))
    ->setBars([['value' => 10], ['value' => 5]])
    ->renderSvg();
echo "waterfall 100k bar label dropped: ", $with === $without ? "ok" : "FAIL", "\n";

$with = (new FastChart\Treemap(300, 200))
    ->setItems([['value' => 10, 'label' => $huge], ['value' => 5]])
    ->renderSvg();
$without = (new FastChart\Treemap(300, 200))
    ->setItems([['value' => 10], ['value' => 5]])
    ->renderSvg();
echo "treemap 100k item label dropped: ", $with === $without ? "ok" : "FAIL", "\n";

// An in-budget label on the same parsers still renders (differs from
// the no-label control), so the cap isn't just dropping everything.
$labeled = (new FastChart\Funnel(300, 200))
    ->setStages([['value' => 10, 'label' => 'Visits'], ['value' => 5]])
    ->renderSvg();
$plain = (new FastChart\Funnel(300, 200))
    ->setStages([['value' => 10], ['value' => 5]])
    ->renderSvg();
echo "funnel small label kept: ", $labeled !== $plain ? "ok" : "FAIL", "\n";

echo "done\n";
?>
--EXPECT--
title 8192 renders: ok
title 8193: ok
xaxis 8193: ok
9000-byte series label dropped: ok
funnel 100k stage label dropped: ok
waterfall 100k bar label dropped: ok
treemap 100k item label dropped: ok
funnel small label kept: ok
done
