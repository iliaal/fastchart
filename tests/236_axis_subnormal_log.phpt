--TEST--
Log axis: a subnormal-positive minimum is rejected instead of producing a NaN pixel cast
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_c293726e: pow(10, floor(log10(5e-324))) underflows to 0, making
 * log_min -Inf and log_span +Inf, so the per-point fraction became NaN and
 * reached the (int) cast (UB). compute_log now rejects the underflow. */

use FastChart\AreaChart;
use FastChart\Chart;

try {
    (new AreaChart())->setSize(400, 300)->setStacked(false)
        ->setYAxisScale(Chart::SCALE_LOG)
        ->setSeries([['name' => 's', 'data' => [5e-324, 1.0, 2.0]]])
        ->renderSvg();
    echo "subnormal_min: rendered\n";
} catch (\Throwable $e) {
    echo "subnormal_min: ",
        (strpos($e->getMessage(), 'strictly-positive') !== false ? 'rejected' : 'threw-other'),
        "\n";
}

$svg = (new AreaChart())->setSize(400, 300)->setStacked(false)
    ->setYAxisScale(Chart::SCALE_LOG)
    ->setSeries([['name' => 's', 'data' => [1.0, 10.0, 100.0]]])
    ->renderSvg();
echo "positive_log_renders: ", (strlen($svg) > 500 ? 'yes' : 'no'), "\n";

?>
--EXPECT--
subnormal_min: rejected
positive_log_renders: yes
