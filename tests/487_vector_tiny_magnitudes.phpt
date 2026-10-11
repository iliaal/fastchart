--TEST--
VectorChart: tiny finite vectors retain relative arrow lengths and colors
--EXTENSIONS--
fastchart
--FILE--
<?php
function field(float $scale, bool $ramp): string {
    $chart = (new FastChart\VectorChart(400, 400))
        ->setXAxisVisible(false)->setYAxisVisible(false)
        ->setVectors([
            ['x' => 0, 'y' => 0, 'dx' => 3 * $scale, 'dy' => 4 * $scale],
            ['x' => 1, 'y' => 1, 'dx' => -6 * $scale, 'dy' => -8 * $scale],
            ['x' => 2, 'y' => 2, 'dx' => 0, 'dy' => 0],
        ]);
    if ($ramp) $chart->setColorRamp(0x000000, 0xFFFFFF);
    return $chart->renderSvg();
}

/* Squaring these finite components either rounds away most precision or
 * underflows to zero. Relative lengths must not depend on their units. */
foreach ([false, true] as $ramp) {
    $reference = field(1.0, $ramp);
    echo $ramp ? "ramp\n" : "palette\n";
    echo 'two nonzero arrows: ', substr_count($reference, '<polygon') === 2 ? 'yes' : 'no', "\n";
    foreach ([2.0 ** -540, 2.0 ** -665, 2.0 ** -996, 2.0 ** -1074] as $scale) {
        echo 'same geometry and colors: ', field($scale, $ramp) === $reference ? 'yes' : 'no', "\n";
    }
}

/* Zero-only fields remain valid and draw no arrowheads. */
$zero = (new FastChart\VectorChart(400, 400))
    ->setVectors([['x' => 0, 'y' => 0, 'dx' => 0, 'dy' => 0]])
    ->renderSvg();
echo 'zero stays invisible: ', !str_contains($zero, '<polygon') ? 'yes' : 'no', "\n";
/* Keep the existing rejection of overflowing squared magnitudes. */
$keeper = ['x' => 1, 'y' => 1, 'dx' => 3, 'dy' => 4];
$chart = new FastChart\VectorChart(400, 400);
$reference = $chart->setVectors([$keeper])->renderSvg();
$withOverflow = $chart->setVectors([
    ['x' => 100, 'y' => 100, 'dx' => 1e200, 'dy' => 0],
    $keeper,
])->renderSvg();
echo 'overflow still filtered: ', $withOverflow === $reference ? 'yes' : 'no', "\n";
?>
--EXPECT--
palette
two nonzero arrows: yes
same geometry and colors: yes
same geometry and colors: yes
same geometry and colors: yes
same geometry and colors: yes
ramp
two nonzero arrows: yes
same geometry and colors: yes
same geometry and colors: yes
same geometry and colors: yes
same geometry and colors: yes
zero stays invisible: yes
overflow still filtered: yes
