--TEST--
LinearMeter / BulletChart: a non-finite range span is rejected (no NaN pointer cast)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_2817d23a: setRange(-PHP_FLOAT_MAX, PHP_FLOAT_MAX) overflowed mx - mn to
 * +inf, making the pointer fraction NaN before the int cast. setRange now
 * rejects a non-finite span on both classes. */

use FastChart\LinearMeter;
use FastChart\BulletChart;

foreach (['LinearMeter' => LinearMeter::class, 'BulletChart' => BulletChart::class] as $name => $cls) {
    try {
        (new $cls())->setRange(-PHP_FLOAT_MAX, PHP_FLOAT_MAX);
        echo "$name: accepted\n";
    } catch (\Throwable $e) {
        echo "$name: rejected\n";
    }
}

?>
--EXPECT--
LinearMeter: rejected
BulletChart: rejected
