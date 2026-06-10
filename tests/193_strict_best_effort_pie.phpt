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
    if (strpos($svg, 'good') === false) {
        echo "MISSING good slice label\n"; $ok = false;
    }
    foreach (['zero', 'neg', 'str'] as $bad) {
        if (strpos($svg, $bad) !== false) {
            echo "BAD slice '$bad' should have been omitted\n"; $ok = false;
        }
    }
    echo "pie_best_effort: ", $ok ? "ok\n" : "FAIL\n";
} catch (Throwable $e) {
    echo "unexpected_throw: ", get_class($e), ": ", $e->getMessage(), "\n";
}
?>
--EXPECT--
pie_best_effort: ok
