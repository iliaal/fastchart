--TEST--
Round-5 audit fixes: clone deep-copies config; renderToFile honors setJpegQuality
--EXTENSIONS--
fastchart
--FILE--
<?php

/* F1 (CRITICAL): clone was Z_TRY_ADDREF'ing self->config so both clones
 * shared the same HashTable. Setters that write into Z_ARRVAL(self->config)
 * (annotations, overlays, horizontal/vertical lines, ...) then triggered
 * a Zend hash assertion (refcount > 1 on update) and aborted the process
 * with exit=134. Fix: recursive deep-copy of config so the outer hash AND
 * every nested list zval (config['annotations'], config['overlays'], ...)
 * is independently owned by the clone. A shallow zend_array_dup() leaves
 * nested lists shared, so a chart that already had an annotation aborted
 * the second add on a clone. */
$c = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => [1, 2, 3, 4, 5]]]);
$svg_pristine = $c->renderSvg();

$c2 = clone $c;
$c2->addTextAnnotation('hello', 50, 50);
echo "clone_addTextAnnotation: ok\n";

$c3 = clone $c;
$c3->addOverlaySeries('line', [10, 20, 30, 40, 50]);
echo "clone_addOverlaySeries: ok\n";

$c4 = clone $c;
$c4->addHorizontalLine(2.5);
echo "clone_addHorizontalLine: ok\n";

$c5 = clone $c;
$c5->addVerticalLine(2.0);
echo "clone_addVerticalLine: ok\n";

/* The original must be unaffected by any of the four mutations. */
echo "clone_isolation: ",
    ($c->renderSvg() === $svg_pristine ? "ok" : "fail"), "\n";

/* Each clone produces a different SVG from the original. */
$mutated_outputs = [$c2->renderSvg(), $c3->renderSvg(),
                    $c4->renderSvg(), $c5->renderSvg()];
$distinct = count(array_unique($mutated_outputs));
echo "clone_mutations_distinct: ",
    ($distinct === 4 && !in_array($svg_pristine, $mutated_outputs, true)
        ? "ok" : "fail"), "\n";

/* Chained mutations on one clone do not bleed into a sibling clone. */
$c6 = clone $c;
$c6->addTextAnnotation('a', 10, 10);
$c7 = clone $c;
$c7->addTextAnnotation('b', 90, 90);
echo "sibling_clones_distinct: ",
    ($c6->renderSvg() !== $c7->renderSvg() ? "ok" : "fail"), "\n";

/* Nested-list scenario the shallow-dup fix missed: seed config first,
 * THEN clone, THEN append. The outer HashTable is independent but
 * config['text_annotations'] / ['overlays'] / ['annotations'] are
 * still shared list zvals between src and dst. The second append on
 * the clone aborted with refcount > 1. Native-text mode lets the
 * test see the literal annotation strings in the SVG (path mode
 * flattens to glyph outlines). */
$seeded = (new FastChart\LineChart(400, 200))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([['data' => [1, 2, 3, 4, 5]]]);
$seeded->addTextAnnotation('first', 10, 10);

$nest = clone $seeded;
$nest->addTextAnnotation('second', 20, 20);
echo "seed_text_then_clone_then_append: ok\n";

/* After the second append, the original must still own 'first' only;
 * the clone must own 'first' + 'second'. Asymmetric containment
 * proves the nested list is no longer shared. */
$seeded->addTextAnnotation('third', 30, 30);
$svg_a = $seeded->renderSvg();
$svg_b = $nest->renderSvg();
$orig_has = substr_count($svg_a, 'first') >= 1
         && substr_count($svg_a, 'third') >= 1
         && substr_count($svg_a, 'second') === 0;
$clone_has = substr_count($svg_b, 'first') >= 1
          && substr_count($svg_b, 'second') >= 1
          && substr_count($svg_b, 'third') === 0;
echo "nested_text_isolation: ",
    ($orig_has && $clone_has ? "ok" : "fail"), "\n";

/* Same scenario for overlays — different config key, different inner
 * list, exercises the deep-copy walk against another nested array. */
$ov = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => [1, 2, 3]]]);
$ov->addOverlaySeries('line', [10, 20, 30]);
$ov2 = clone $ov;
$ov2->addOverlaySeries('line', [40, 50, 60]);
echo "seed_overlay_then_clone_then_append: ok\n";

/* And for h-line / v-line, which share the 'annotations' list. */
$hv = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => [1, 2, 3]]]);
$hv->addHorizontalLine(1.5);
$hv2 = clone $hv;
$hv2->addVerticalLine(2.0);  /* same list, different kind */
echo "seed_hline_clone_append_vline: ok\n";

/* Clone-of-clone: deep copy must hold across multiple generations. */
$g1 = clone $hv;
$g1->addHorizontalLine(2.5);
$g2 = clone $g1;
$g2->addHorizontalLine(3.5);
echo "grandchild_clone_append: ok\n";

/* F2 (Important): renderToFile('*.jpg') ignored setJpegQuality() because
 * the default $quality=90 made the q_eff==0 fallback unreachable. Fix:
 * default $quality=0 as sentinel; format-specific dispatch promotes 0 to
 * self->jpeg_quality for JPEG (or 90 for WebP). */
$tmp_jpg = sys_get_temp_dir() . '/fc_test_round5.jpg';

$c = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => [1, 2, 3, 4, 5]]])
    ->setJpegQuality(1);

$ram = $c->renderJpeg();
$c->renderToFile($tmp_jpg);
$disk = file_get_contents($tmp_jpg);
echo "chart_renderToFile_uses_jpegQuality: ",
    (abs(strlen($ram) - strlen($disk)) < 50 ? "ok" : "fail"), "\n";

/* Explicit quality argument still works and overrides the setter. */
$c->renderToFile($tmp_jpg, 90);
$disk_q90 = filesize($tmp_jpg);
echo "chart_explicit_quality_wins: ",
    ($disk_q90 > strlen($disk) * 2 ? "ok" : "fail"), "\n";

/* Explicit quality argument that matches setter produces same output. */
$c->renderToFile($tmp_jpg, 1);
echo "chart_explicit_quality_1: ",
    (abs(filesize($tmp_jpg) - strlen($ram)) < 50 ? "ok" : "fail"), "\n";

/* Out-of-range quality still rejected. */
try {
    $c->renderToFile($tmp_jpg, 101);
    echo "chart_quality_oob_accepted: REGRESSION\n";
} catch (\ValueError $e) {
    echo "chart_quality_oob_rejected: ok\n";
}
try {
    $c->renderToFile($tmp_jpg, -1);
    echo "chart_quality_neg_accepted: REGRESSION\n";
} catch (\ValueError $e) {
    echo "chart_quality_neg_rejected: ok\n";
}

/* Same on Symbol side: setJpegQuality(1) must apply to renderToFile. */
$q = (new FastChart\QrCode(200, 200))
    ->setData('hello world')
    ->setJpegQuality(1);
$ram_q = $q->renderJpeg();
$q->renderToFile($tmp_jpg);
echo "symbol_renderToFile_uses_jpegQuality: ",
    (abs(strlen($ram_q) - filesize($tmp_jpg)) < 50 ? "ok" : "fail"), "\n";

/* Symbol explicit quality wins. */
$q->renderToFile($tmp_jpg, 90);
echo "symbol_explicit_quality_wins: ",
    (filesize($tmp_jpg) > strlen($ram_q) * 2 ? "ok" : "fail"), "\n";

@unlink($tmp_jpg);

echo "ok\n";
?>
--EXPECT--
clone_addTextAnnotation: ok
clone_addOverlaySeries: ok
clone_addHorizontalLine: ok
clone_addVerticalLine: ok
clone_isolation: ok
clone_mutations_distinct: ok
sibling_clones_distinct: ok
seed_text_then_clone_then_append: ok
nested_text_isolation: ok
seed_overlay_then_clone_then_append: ok
seed_hline_clone_append_vline: ok
grandchild_clone_append: ok
chart_renderToFile_uses_jpegQuality: ok
chart_explicit_quality_wins: ok
chart_explicit_quality_1: ok
chart_quality_oob_rejected: ok
chart_quality_neg_rejected: ok
symbol_renderToFile_uses_jpegQuality: ok
symbol_explicit_quality_wins: ok
ok
