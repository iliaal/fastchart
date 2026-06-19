--TEST--
Waterfall: a negative TOTAL bar renders at its absolute height
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* Regression: a TOTAL bar stored bar_lo=0, bar_hi=value assuming
 * value >= 0. A negative total (a net-loss subtotal) had bar_hi < 0,
 * which the Y-range scan ignored, so the render clamped it to the
 * baseline (an invisible zero-height bar) and mis-scaled the axis. Per
 * the documented contract a TOTAL renders at its absolute height, so a
 * -50 total must render identically to a +50 total. */

function wf($v): string {
    return (new FastChart\Waterfall(400, 300))
        ->setBars([['label' => 'Net', 'value' => $v, 'kind' => 'total']])
        ->renderSvg();
}
function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$neg = wf(-50);
$pos = wf(50);
echo "neg_valid: ", valid($neg) ? "yes" : "no", "\n";
echo "neg_equals_pos_abs: ", ($neg === $pos) ? "yes" : "no", "\n";

echo "ok\n";
?>
--EXPECT--
neg_valid: yes
neg_equals_pos_abs: yes
ok
