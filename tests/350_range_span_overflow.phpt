--TEST--
GaugeChart::setRange rejects a non-finite span (no NaN pointer cast)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Both min and max are finite, but mx - mn overflows to +inf. Downstream
 * gauge fraction math then divides by that infinite span and the NaN result
 * reaches an int cast (the UB class v1.1.1 closed on LinearMeter /
 * BulletChart::setRange). GaugeChart::setRange now rejects a non-finite span
 * too, matching its Meter / Bullet siblings.
 *
 * Chart::setYAxisRange is deliberately NOT in scope here: it accepts an
 * overflowing span and clamps at render time in the axis layer
 * (see 287_axis_forced_range_overflow_cast). */

use FastChart\GaugeChart;

function outcome(callable $fn): string {
    try {
        $fn();
        return "accepted";
    } catch (\ValueError $e) {
        return "rejected";
    }
}

echo "gauge span:   ", outcome(fn() => (new GaugeChart())->setRange(-PHP_FLOAT_MAX, PHP_FLOAT_MAX)), "\n";
echo "gauge normal: ", outcome(fn() => (new GaugeChart())->setRange(0, 100)), "\n";

?>
--EXPECT--
gauge span:   rejected
gauge normal: accepted
