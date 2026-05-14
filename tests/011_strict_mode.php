<?php

$canvas = function () { return imagecreatetruecolor(400, 200); };

// Default (non-strict): silent skip.
$im = $canvas();
$out = (new FastChart\LineChart)
    ->setSize(400, 200)
    ->setSeries([1, "two", 3, null, 5])
    ->draw($im);
var_dump($out instanceof \GdImage);

// Strict: TypeError.
foreach ([
    'LineChart' => fn() => (new FastChart\LineChart)->setStrict(true)->setSize(400, 200)->setSeries([1, "x"])->draw($canvas()),
    'BarChart'  => fn() => (new FastChart\BarChart)->setStrict(true)->setSize(400, 200)->setSeries(["bad"])->draw($canvas()),
] as $name => $fn) {
    try {
        $fn();
        echo "$name strict: no throw\n";
    } catch (\TypeError $e) {
        echo "$name strict: ", str_contains($e->getMessage(), "strict") ? "TypeError ok" : "wrong message", "\n";
    }
}

// Toggle off again.
$im = $canvas();
(new FastChart\LineChart)
    ->setStrict(true)
    ->setStrict(false)
    ->setSize(400, 200)
    ->setSeries([1, "ok-skipped", 3])
    ->draw($im);
echo "strict_toggle_off: ok\n";
?>
