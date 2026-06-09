--TEST--
CalendarHeatmap::setData(): pre-1970 dates are kept, malformed keys dropped
--EXTENSIONS--
fastchart
--FILE--
<?php

/* "1969-12-31" is day index -1 — the same value the date parser used
 * as its failure sentinel. Pre-fix every pre-1970 key was silently
 * dropped; an all-historical dataset then made draw() throw the
 * misleading "requires setData() with at least one entry". */

/* All-pre-1970 data must render. */
$c = (new FastChart\CalendarHeatmap(600, 200))
    ->setData([
        '1969-12-31' => 5,
        '1969-12-30' => 3,
        '1969-11-01' => 1,
    ]);
$svg = $c->renderSvg();
var_dump(strlen($svg) > 0);

/* Mixed: the epoch-straddling span renders as one grid. */
$c2 = (new FastChart\CalendarHeatmap(600, 200))
    ->setData([
        '1969-12-29' => 1,
        '1970-01-02' => 2,
    ]);
var_dump(strlen($c2->renderSvg()) > 0);

/* Malformed keys must still be dropped — with ONLY bad keys the
 * draw-time guard fires exactly as before. */
$c3 = (new FastChart\CalendarHeatmap(600, 200))
    ->setData([
        'not-a-date'  => 1,
        '2026-02-30'  => 2,
        '2026-13-01'  => 3,
    ]);
try {
    $c3->renderSvg();
    echo "no error\n";
} catch (Error $e) {
    echo $e->getMessage(), "\n";
}

?>
--EXPECT--
bool(true)
bool(true)
FastChart\CalendarHeatmap::draw() requires setData() with at least one entry
