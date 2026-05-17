--TEST--
Funnel STYLE_CONE: wide-and-short canvas throws cleanly instead of div-by-zero
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: fastchart_funnel.c:141 — in CONE mode the layout
 * subtracts cone_bottom_reserve (≈22% of max_half) from (y1 - y0)
 * to form total_h. A wide-and-short canvas (e.g. W=1890, H=210)
 * produces max_half≈845, reserve≈186, y1-y0≈186 → total_h == 0.
 * The first per-stage iteration then divides 0.0/0.0=NaN, casts
 * NaN to int (undefined per C11 6.3.1.4p1), and feeds the result
 * into ellipse-arc rendering. Fix: throw a clean error when
 * total_h <= 0 in CONE mode, mirroring the canvas-too-narrow
 * guard at line 71. */

$threw = false;
$msg = '';
try {
    $svg = (new FastChart\Funnel(1890, 210))
        ->setStyle(FastChart\Funnel::STYLE_CONE)
        ->setStages([
            ['label' => 'Visited',   'value' => 1000],
            ['label' => 'Signed up', 'value' => 300],
            ['label' => 'Purchased', 'value' => 50],
        ])
        ->renderSvg();
} catch (\Throwable $e) {
    $threw = true;
    $msg = $e->getMessage();
}

echo "throws_clean_error: ", ($threw ? "ok" : "BUG: silent UB-render"), "\n";
echo "message_mentions_canvas: ",
    (stripos($msg, 'canvas') !== false || stripos($msg, 'short') !== false || stripos($msg, 'too') !== false ? "ok" : "BAD ($msg)"), "\n";

/* Sanity: a normally-proportioned canvas still works in CONE. */
$svg2 = (new FastChart\Funnel(600, 500))
    ->setStyle(FastChart\Funnel::STYLE_CONE)
    ->setStages([
        ['label' => 'Visited',   'value' => 1000],
        ['label' => 'Signed up', 'value' => 300],
        ['label' => 'Purchased', 'value' => 50],
    ])
    ->renderSvg();
echo "normal_cone_renders: ",
    (strlen($svg2) > 1000 ? "ok" : "BAD"), "\n";

echo "done\n";
?>
--EXPECT--
throws_clean_error: ok
message_mentions_canvas: ok
normal_cone_renders: ok
done
