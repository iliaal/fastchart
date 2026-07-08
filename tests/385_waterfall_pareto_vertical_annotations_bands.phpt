--TEST--
Waterfall and Pareto honor vertical annotations and plot bands
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php

function has_hex(string $svg, string $hex): string {
    return str_contains($svg, $hex) ? 'yes' : 'no';
}
function has_rgba(string $svg, string $rgb): string {
    return str_contains($svg, 'fill="rgba(' . $rgb . ',') ? 'yes' : 'no';
}

$wf = (new FastChart\Waterfall(520, 320))
    ->setBars([
        ['label' => 'a', 'value' => 10],
        ['label' => 'b', 'value' => 5],
        ['label' => 'c', 'value' => -3],
    ])
    ->addHorizontalBand(3, 8, 0x123456, 40)
    ->addVerticalBand(0.5, 1.5, 0x654321, 40)
    ->addVerticalLine(1.0, 'marker', 0xABCDEF);
$wf_svg = $wf->renderSvg();
echo "waterfall hband: ", has_rgba($wf_svg, '18,52,86'), "\n";
echo "waterfall vband: ", has_rgba($wf_svg, '101,67,33'), "\n";
echo "waterfall vline: ", has_hex($wf_svg, '#ABCDEF'), "\n";

$pareto = (new FastChart\ParetoChart(520, 320))
    ->setBars([
        ['label' => 'A', 'value' => 50],
        ['label' => 'B', 'value' => 30],
        ['label' => 'C', 'value' => 20],
    ])
    ->addHorizontalBand(15, 35, 0x123456, 40)
    ->addVerticalBand(0.5, 1.5, 0x654321, 40)
    ->addVerticalLine(1.0, 'marker', 0xABCDEF);
$pareto_svg = $pareto->renderSvg();
echo "pareto hband: ", has_rgba($pareto_svg, '18,52,86'), "\n";
echo "pareto vband: ", has_rgba($pareto_svg, '101,67,33'), "\n";
echo "pareto vline: ", has_hex($pareto_svg, '#ABCDEF'), "\n";

?>
--EXPECT--
waterfall hband: yes
waterfall vband: yes
waterfall vline: yes
pareto hband: yes
pareto vband: yes
pareto vline: yes
