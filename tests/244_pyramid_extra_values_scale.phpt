--TEST--
PopulationPyramid: data points beyond the category count do not skew the bar scale
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_8f292130: the value scale iterated the full series, so a trailing value
 * past cat_count (never drawn) could dominate max_val and collapse the visible
 * bars. The scale is now limited to the drawn rows. */

use FastChart\PopulationPyramid;

function pyramid(array $left): string {
    return (new PopulationPyramid())->setSize(400, 300)
        ->setCategories(['A', 'B'])
        ->setLeftSeries(['data' => $left])
        ->setRightSeries(['data' => [12, 22]])
        ->renderSvg();
}

/* The trailing 1e6 (index 2, beyond the 2 categories) must be ignored, so the
 * render is identical to the two-value series. */
echo "extra_value_ignored: ",
    (pyramid([10, 20]) === pyramid([10, 20, 1000000]) ? 'yes' : 'no'), "\n";

?>
--EXPECT--
extra_value_ignored: yes
