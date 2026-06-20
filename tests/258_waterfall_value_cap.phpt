--TEST--
Waterfall: out-of-cap bar values are dropped so the cumulative can't overflow to invalid geometry
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_99168b97: setBars() applied no magnitude cap, so PHP_FLOAT_MAX values let
 * the running cumulative overflow to +inf and emit invalid SVG like
 * height="-254". Values are now capped like the other data parsers. */

use FastChart\Waterfall;

/* The over-cap middle value is dropped; the two in-range bars render with no
 * negative-height rectangle. */
$svg = (new Waterfall())->setSize(400, 200)
    ->setBars([
        ['label' => 'a', 'value' => 100],
        ['label' => 'b', 'value' => PHP_FLOAT_MAX],
        ['label' => 'c', 'value' => 50],
    ])
    ->renderSvg();
echo "over_cap_no_overflow: ", (preg_match('/height="-\d/', $svg) ? 'no' : 'yes'), "\n";

?>
--EXPECT--
over_cap_no_overflow: yes
