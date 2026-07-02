--TEST--
Funnel: strict setStages() rejects bad stages; short canvas is rejected
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression 1: setStages() ignored the inherited strict flag and
 * silently skipped malformed/out-of-range stages, so a strict caller
 * still got a partial chart. It now parses into a temp and throws in
 * strict mode without touching existing state.
 * Regression 2: the flat funnel positions bands at y0 + i*stage_h with a
 * 4px floor; a canvas too short for the stage count spilled later bands
 * past the bottom edge. It now rejects instead of emitting clipped bands. */

/* strict: invalid stage throws, state untouched */
try {
    (new FastChart\Funnel(300, 300))->setStrict(true)
        ->setStages([['value' => 10], ['value' => -5]]);
    echo "strict_bad: no-throw\n";
} catch (\Throwable $e) {
    echo "strict_bad: threw\n";
}

/* non-strict: invalid stage silently dropped, still renders */
$f = (new FastChart\Funnel(300, 300))
    ->setStages([['value' => 10], ['value' => -5]])->renderSvg();
echo "nonstrict_bad: ", (strlen($f) > 100 ? "renders" : "BAD"), "\n";

/* strict: all-valid passes */
try {
    $g = (new FastChart\Funnel(300, 300))->setStrict(true)
        ->setStages([['value' => 10], ['value' => 6]])->renderSvg();
    echo "strict_good: ", (strlen($g) > 100 ? "renders" : "BAD"), "\n";
} catch (\Throwable $e) {
    echo "strict_good: threw\n";
}

/* short canvas: 6 stages cannot fit in a 20px-tall plot */
$stages = [];
for ($i = 0; $i < 6; $i++) $stages[] = ['value' => 10 - $i];
try {
    (new FastChart\Funnel(300, 40))->setStages($stages)->renderSvg();
    echo "short_canvas: no-throw\n";
} catch (\Throwable $e) {
    echo "short_canvas: threw\n";
}

?>
--EXPECT--
strict_bad: threw
nonstrict_bad: renders
strict_good: renders
short_canvas: threw
