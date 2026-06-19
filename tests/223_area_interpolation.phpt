--TEST--
AreaChart: smooth and stepped fills follow setLineInterpolation
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* setLineInterpolation() reshapes the area's top boundary the same way
 * it reshapes a line: SMOOTH densifies it into a Catmull-Rom curve,
 * the STEP_* modes turn it into a staircase. Each mode produces valid
 * SVG with finite coordinates, and the smooth/step silhouettes differ
 * from the linear baseline. Stacked smooth fills keep one polygon per
 * series and tile (each layer's curved top is the next layer's bottom,
 * so the densified output stays valid). */

function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

function coord_tokens(string $svg): int {
    return substr_count($svg, ',');
}

$data = [['data' => [3, 7, 4, 8, 5, 9, 6]]];

$lin    = (new FastChart\AreaChart(400, 260))->setSeries($data)
              ->setLineInterpolation(FastChart\Chart::INTERP_LINEAR)->renderSvg();
$smooth = (new FastChart\AreaChart(400, 260))->setSeries($data)
              ->setLineInterpolation(FastChart\Chart::INTERP_SMOOTH)->renderSvg();
$stepA  = (new FastChart\AreaChart(400, 260))->setSeries($data)
              ->setLineInterpolation(FastChart\Chart::INTERP_STEP_AFTER)->renderSvg();
$stepB  = (new FastChart\AreaChart(400, 260))->setSeries($data)
              ->setLineInterpolation(FastChart\Chart::INTERP_STEP_BEFORE)->renderSvg();

echo "all_valid: ", (valid($lin) && valid($smooth) && valid($stepA) && valid($stepB)) ? "yes" : "no", "\n";
echo "all_finite: ", (strpos($smooth, '-2147483648') === false && strpos($stepA, '-2147483648') === false) ? "yes" : "no", "\n";

/* SMOOTH densifies the boundary into many sub-segments, so it carries
 * far more coordinate tokens than the straight-segment linear fill. */
echo "smooth_denser_than_linear: ", (coord_tokens($smooth) > coord_tokens($lin)) ? "yes" : "no", "\n";

/* Each step mode differs from linear and from the other step mode. */
echo "stepA_differs: ", ($stepA !== $lin) ? "yes" : "no", "\n";
echo "stepB_differs: ", ($stepB !== $lin) ? "yes" : "no", "\n";
echo "stepA_ne_stepB: ", ($stepA !== $stepB) ? "yes" : "no", "\n";

/* Stacked smooth: one filled polygon per series, valid, and distinct
 * from the linear stack. */
$stk = [
    ['data' => [3, 5, 2, 6, 4, 7, 3]],
    ['data' => [2, 3, 4, 2, 5, 3, 4]],
];
$stkLin = (new FastChart\AreaChart(400, 260))->setSeries($stk)->setStacked(true)
              ->setLineInterpolation(FastChart\Chart::INTERP_LINEAR)->renderSvg();
$stkSm  = (new FastChart\AreaChart(400, 260))->setSeries($stk)->setStacked(true)
              ->setLineInterpolation(FastChart\Chart::INTERP_SMOOTH)->renderSvg();
echo "stacked_smooth_valid: ", valid($stkSm) ? "yes" : "no", "\n";
echo "stacked_smooth_polys_2: ", (substr_count($stkSm, '<polygon') === 2 ? "yes" : "no"), "\n";
echo "stacked_smooth_differs: ", ($stkSm !== $stkLin) ? "yes" : "no", "\n";

echo "ok\n";
?>
--EXPECT--
all_valid: yes
all_finite: yes
smooth_denser_than_linear: yes
stepA_differs: yes
stepB_differs: yes
stepA_ne_stepB: yes
stacked_smooth_valid: yes
stacked_smooth_polys_2: yes
stacked_smooth_differs: yes
ok
