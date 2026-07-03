--TEST--
Funnel::setStages() all-invalid re-set clears previously parsed stages
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression (fnd_2da83a89): a second setStages() call with a non-empty
 * array whose entries all fail validation (non-strict) returned early on
 * idx==0 without freeing the prior stages, so the stale stages rendered.
 * Siblings Pareto/Scatter free-first; Funnel must clear too. Both the
 * empty-array re-set and the all-invalid re-set must leave no stages. */

function stage_count_via_draw(FastChart\Funnel $f): string {
    try { $f->renderSvg(); return "rendered"; }
    catch (\Throwable $e) { return "empty"; }
}

$f = new FastChart\Funnel(300, 300);
$f->setStages([['value' => 10, 'label' => 'A'], ['value' => 6, 'label' => 'B']]);
echo "initial: ", stage_count_via_draw($f), "\n";

// All-invalid non-empty re-set: must clear, not keep [A, B].
$f->setStages([['value' => -5], ['value' => 0], ['nope' => 1]]);
echo "after all-invalid re-set: ", stage_count_via_draw($f), "\n";

// Empty re-set on a fresh chart also clears (control).
$g = new FastChart\Funnel(300, 300);
$g->setStages([['value' => 3, 'label' => 'X']]);
$g->setStages([]);
echo "after empty re-set: ", stage_count_via_draw($g), "\n";

?>
--EXPECT--
initial: rendered
after all-invalid re-set: empty
after empty re-set: empty
