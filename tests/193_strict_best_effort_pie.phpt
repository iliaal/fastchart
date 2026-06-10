--TEST--
PieChart (and other non-Line families) treat setStrict(true) as best-effort
--EXTENSIONS--
fastchart
--FILE--
<?php
// A bad slice (non-positive value or non-numeric) must be silently
// dropped even under setStrict(true). No TypeError. Good slices
// must still render.
$input = [
    'good' => 40,
    'zero' => 0,
    'neg'  => -3,
    'str'  => 'not-a-number',
];

$ok = true;
try {
    $p = (new FastChart\PieChart(300, 300))
        ->setStrict(true)
        ->setSlices($input);
    $svg = $p->renderSvg();
    /* Contract test: setStrict(true) on Pie (best-effort family) must not
     * throw TypeError, even with bad data. Render must succeed. */
    echo "pie_best_effort: ok\n";
} catch (Throwable $e) {
    echo "unexpected_throw: ", get_class($e), ": ", $e->getMessage(), "\n";
}
?>
--EXPECT--
pie_best_effort: ok
