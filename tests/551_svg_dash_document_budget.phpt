--TEST--
svgToPng: stroke-dash expansion is budgeted per document, not per path
--EXTENSIONS--
fastchart
--FILE--
<?php

function dashed_lines(int $n, string $len, string $dash, int $size = 1): string
{
    return '<svg xmlns="http://www.w3.org/2000/svg" width="' . $size
        . '" height="' . $size . '">'
        . str_repeat('<line x1="0" y1="0" x2="' . $len . '" y2="0" '
            . 'stroke="#000" stroke-dasharray="' . $dash . '"/>', $n)
        . '</svg>';
}

/* The document budget is 2^18 dash commands, sized so dashes can never
 * exhaust the stroke outline budget first. Each 12000 px line of 0.2
 * dashes needs ~60000 commands: four fit and render. */
$png = FastChart\Chart::svgToPng(dashed_lines(4, '12000', '0.2'));
echo "four_within_budget: ", (strlen($png) > 0 ? "ok" : "fail"), "\n";

/* A fifth exceeds it. The budget used to be invisible to the element x
 * pixel work budget (5 elements on a 1x1 canvas). */
try {
    FastChart\Chart::svgToPng(dashed_lines(5, '12000', '0.2'));
    echo "over_budget: rendered\n";
} catch (\ValueError $e) {
    echo "over_budget: ", $e->getMessage(), "\n";
}

/* One path at plutovg's 2^20 per-path cap is now over the document
 * budget on its own. */
try {
    FastChart\Chart::svgToPng(dashed_lines(1, '1e6', '0.2'));
    echo "single_path_at_cap: rendered\n";
} catch (\ValueError $e) {
    echo "single_path_at_cap: ", $e->getMessage(), "\n";
}

/* Ordinary dashed strokes are far below the budget. */
$png = FastChart\Chart::svgToPng(dashed_lines(200, '400', '4,3', 400));
echo "ordinary_dashes: ", (strlen($png) > 0 ? "ok" : "fail"), "\n";
?>
--EXPECT--
four_within_budget: ok
over_budget: FastChart\Chart::svgToPng() SVG render work exceeds cap (stroke dash expansion too large)
single_path_at_cap: FastChart\Chart::svgToPng() SVG render work exceeds cap (stroke dash expansion too large)
ordinary_dashes: ok
