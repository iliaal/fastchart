--TEST--
Series parsing: holed arrays compact in order; series/strict caps throw
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Review fixes: arrays with holes (array_filter/unset) were walked by
 * packed index, silently turning missing indexes into NaN gaps and
 * dropping tail values even under setStrict(true); more than 8 series
 * silently truncated; strict mode ignored non-array series entries and
 * malformed floating pairs. */

use FastChart\BarChart;
use FastChart\LineChart;

/* Holes compact in order: array_filter drops the 0 but keeps 30. */
$holed = array_filter([10, 0, 20, 30]);   /* keys 0,2,3 */
$svg_holed  = (new LineChart(300, 200))->setSeries($holed)->renderSvg();
$svg_packed = (new LineChart(300, 200))->setSeries([10, 20, 30])->renderSvg();
echo "holes_compact: ", ($svg_holed === $svg_packed ? 'yes' : 'NO'), "\n";

/* unset($s[0]) on a multi-series list no longer breaks detection. */
$multi = [
    ['data' => [1, 2, 3]],
    ['data' => [4, 5, 6]],
];
unset($multi[0]);
$a = (new LineChart(300, 200))->setSeries($multi)->renderSvg();
$b = (new LineChart(300, 200))->setSeries([['data' => [4, 5, 6]]])->renderSvg();
echo "unset_first_multi: ", ($a === $b ? 'yes' : 'NO'), "\n";

/* More than 8 series throws instead of silently truncating. */
$nine = [];
for ($i = 0; $i < 9; $i++) $nine[] = ['data' => [1, 2, 3]];
try {
    (new LineChart(300, 200))->setSeries($nine);
    echo "nine_series: NO THROW\n";
} catch (ValueError $e) {
    echo "nine_series: throws (", str_contains($e->getMessage(), 'at most') ? 'ok' : 'BAD MSG', ")\n";
}

/* Strict mode: non-array series entry throws. */
try {
    (new LineChart(300, 200))->setStrict(true)
        ->setSeries([['data' => [1, 2]], "junk"]);
    echo "strict_non_array: NO THROW\n";
} catch (TypeError $e) {
    echo "strict_non_array: throws\n";
}

/* Strict mode: malformed floating pair throws. */
try {
    (new BarChart(300, 200))->setStrict(true)->setFloating(true)
        ->setSeries([[1, 5], "junk", [2, 6]]);
    echo "strict_floating: NO THROW\n";
} catch (TypeError $e) {
    echo "strict_floating: throws\n";
}

/* Lax mode keeps prior behavior: malformed floating entries become gaps. */
$svg = (new BarChart(300, 200))->setFloating(true)
    ->setSeries([[1, 5], "junk", [2, 6]])->renderSvg();
echo "lax_floating_renders: ", (strlen($svg) > 500 ? 'yes' : 'NO'), "\n";

?>
--EXPECT--
holes_compact: yes
unset_first_multi: yes
nine_series: throws (ok)
strict_non_array: throws
strict_floating: throws
lax_floating_renders: yes
