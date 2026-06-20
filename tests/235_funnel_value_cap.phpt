--TEST--
Funnel: out-of-cap stage values are dropped, in-range values render (no float-cast UB)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_8d1df6a8: setStages() applied no magnitude cap, so a value like 1e308
 * overflowed max_half * v / max_v to +inf before the int cast (UB). The cap
 * now drops such values like the pack/graph parsers. */

use FastChart\Funnel;

try {
    (new Funnel())->setSize(400, 300)
        ->setStages([['value' => 1e308], ['value' => 1e308]])
        ->renderSvg();
    echo "all_over_cap: rendered\n";
} catch (\Throwable $e) {
    echo "all_over_cap: throws\n";
}

$svg = (new Funnel())->setSize(400, 300)
    ->setStages([['value' => 1e12], ['value' => 5e11]])
    ->renderSvg();
echo "near_cap_renders: ", (strpos($svg, '<polygon') !== false ? 'yes' : 'no'), "\n";

?>
--EXPECT--
all_over_cap: throws
near_cap_renders: yes
