--TEST--
Waterfall: a negative TOTAL bar renders below the zero baseline
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* A TOTAL bar is anchored to zero but keeps its sign: a net-loss
 * subtotal (a negative total) draws from the value up to the baseline,
 * below the axis, the way waterfall charts conventionally show a
 * negative cumulative total — not fabs()'d to render as a positive-height
 * bar identical to its absolute value. The signed span pulls the y-axis
 * into the negative region. */

function wf($v): string {
    return (new FastChart\Waterfall(400, 300))
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setBars([['label' => 'Net', 'value' => $v, 'kind' => 'total']])
        ->renderSvg();
}
function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}
function has_negative_axis(string $svg): bool {
    return (bool) preg_match('/<text[^>]*>-\d/', $svg);
}

$neg = wf(-50);
$pos = wf(50);

echo "neg_valid: ", valid($neg) ? "yes" : "no", "\n";
echo "neg_axis_below_zero: ", has_negative_axis($neg) ? "yes" : "no", "\n";
echo "pos_axis_all_positive: ", has_negative_axis($pos) ? "no" : "yes", "\n";
echo "neg_differs_from_pos: ", ($neg !== $pos) ? "yes" : "no", "\n";

echo "ok\n";
?>
--EXPECT--
neg_valid: yes
neg_axis_below_zero: yes
pos_axis_all_positive: yes
neg_differs_from_pos: yes
ok
