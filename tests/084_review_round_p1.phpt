--TEST--
Review-round P1: silent-feature drops, NUL-axis, truncation, secondary-axis range
--EXTENSIONS--
fastchart
--FILE--
<?php

/* P1 #2 — BoxPlot now renders annotations + overlays. */
$im = imagecreatetruecolor(400, 250);
$out = (new FastChart\BoxPlot(400, 250))
    ->setBoxes([
        ['min' => 1, 'q1' => 3, 'median' => 5, 'q3' => 7, 'max' => 9],
        ['min' => 2, 'q1' => 4, 'median' => 6, 'q3' => 8, 'max' => 10],
    ])
    ->addHorizontalLine(5.0, "median", 0xFF0000)
    ->addVerticalLine(0.0, "first", 0x0000FF)
    ->addOverlaySeries('line', [4.5, 5.5])
    ->draw($im);
echo "boxplot_overlays_annotations: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* P1 #3 — BubbleChart now renders v-line annotations. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BubbleChart(400, 200))
    ->setPoints([[1.0, 10.0, 5.0], [2.0, 20.0, 8.0], [3.0, 15.0, 6.0]])
    ->addHorizontalLine(15.0, "ref")
    ->addVerticalLine(2.0, "x-ref")
    ->draw($im);
echo "bubble_v_annotation: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* P1 #4 — Bar horizontal renders overlays. */
$im = imagecreatetruecolor(400, 250);
$out = (new FastChart\BarChart(400, 250))
    ->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)
    ->setSeries([['data' => [10, 20, 30, 25]]])
    ->addOverlaySeries('line', [12, 22, 28, 24])
    ->draw($im);
echo "h_bar_overlays: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* P1 #5 — secondary Y-axis range override. setYAxisRange should
 * affect the right axis when a series targets it. Just smoke that
 * it renders — the visual change is hard to assert without pixel
 * inspection. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\LineChart(400, 200))
    ->setSecondaryYAxis(true)
    ->setSeries([
        ['data' => [10, 20, 30]],
        ['data' => [100, 200, 300], 'axis' => 'right'],
    ])
    ->setYAxisRange(0.0, 1000.0)
    ->draw($im);
echo "secondary_axis_override: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* P1 #6 — NUL-aware axis selection. "right\0" should NOT bind to
 * the right axis. With strict-mode off, the silent reject should
 * leave the series on the left axis (smoke test: it renders). */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\LineChart(400, 200))
    ->setSecondaryYAxis(true)
    ->setSeries([
        ['data' => [10, 20, 30], 'axis' => "right\0junk"],
    ])
    ->draw($im);
echo "nul_axis_silent_reject: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* P1 #7 — LineChart rejects > 2048 points per series instead of
 * silently truncating. */
try {
    (new FastChart\LineChart(400, 200))
        ->setSeries([['data' => array_fill(0, 2049, 1.0)]]);
    echo "oversize_series: no throw\n";
} catch (\ValueError $e) {
    echo "oversize_series: ValueError ok\n";
}

/* But 2048 exactly is accepted. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\LineChart(400, 200))
    ->setSeries([['data' => array_fill(0, 2048, 5.0)]])
    ->draw($im);
echo "max_size_series: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* P1 #8 — Gradient LUT cache: render with gradient enabled, just
 * smoke that it renders. The perf win is invisible to the test. */
$im = imagecreatetruecolor(400, 200);
$out = (new FastChart\BarChart(400, 200))
    ->setSeries([['data' => [10, 20, 30, 25, 18, 12, 30, 22, 14, 28]]])
    ->setGradientFill(0xFFAA00, 0xFF0000, FastChart\Chart::GRADIENT_VERTICAL)
    ->draw($im);
echo "gradient_render: ", ($out instanceof \GdImage ? "ok" : "bad"), "\n";

/* P1 #9 — clone deep-copies for gantt / boxplot / bubble. Triggering
 * clone via PHP's clone keyword exercises the addref helpers; if any
 * fail to deep-copy, the second draw would double-free on dtor. */
$gantt = (new FastChart\GanttChart(400, 200))
    ->setTimeRange(1700000000, 1700000000 + 30 * 86400)
    ->setTasks([
        ['name' => 'design', 'start' => 1700000000, 'end' => 1700086400 * 5],
        ['name' => 'build',  'start' => 1700086400 * 5, 'end' => 1700086400 * 15],
    ]);
$g2 = clone $gantt;
$im = imagecreatetruecolor(400, 200);
echo "gantt_clone: ", ($g2->draw($im) instanceof \GdImage ? "ok" : "bad"), "\n";
unset($gantt, $g2);

$box = (new FastChart\BoxPlot(400, 200))
    ->setBoxes([
        ['min'=>1,'q1'=>3,'median'=>5,'q3'=>7,'max'=>9,
         'outliers'=>[0.5, 9.5], 'label'=>'A'],
    ]);
$b2 = clone $box;
$im = imagecreatetruecolor(400, 200);
echo "boxplot_clone: ", ($b2->draw($im) instanceof \GdImage ? "ok" : "bad"), "\n";
unset($box, $b2);

$bub = (new FastChart\BubbleChart(400, 200))
    ->setPoints([[1.0, 10.0, 5.0], [2.0, 20.0, 8.0]]);
$b3 = clone $bub;
$im = imagecreatetruecolor(400, 200);
echo "bubble_clone: ", ($b3->draw($im) instanceof \GdImage ? "ok" : "bad"), "\n";
?>
--EXPECT--
boxplot_overlays_annotations: ok
bubble_v_annotation: ok
h_bar_overlays: ok
secondary_axis_override: ok
nul_axis_silent_reject: ok
oversize_series: ValueError ok
max_size_series: ok
gradient_render: ok
gantt_clone: ok
boxplot_clone: ok
bubble_clone: ok
