--TEST--
ParetoChart distributes remainder pixels and keeps narrow bars inside their slots
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
$probe = (new FastChart\ParetoChart(300, 200))
    ->setBars([['label' => 'font-probe', 'value' => 1]])
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)->renderSvg();
if (!str_contains($probe, '>font-probe</text>')) die('skip no usable default font');
?>
--FILE--
<?php
function checkSlots(int $span, int $count): void {
    $data = [];
    for ($i = 0; $i < $count; $i++) {
        $data[] = ['label' => "category$i", 'value' => 1, 'color' => 0x123456];
    }
    $chart = (new FastChart\ParetoChart(600, 300))
        ->setPlotRect(31, 20, 31 + $span, 250)
        ->setBars($data)
        ->setShowValues(true)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setLineColor(0xFEDCBA)
        ->addVerticalLine($count - 1, '', 0xABCDEF);
    try {
        $svg = $chart->renderSvg();
    } catch (ValueError $e) {
        echo "$span/$count: ", $span < $count ? 'rejected' : 'unexpected rejection', "\n";
        return;
    }
    preg_match_all('/<rect x="(\d+)" y="\d+" width="(\d+)" height="\d+" fill="#123456"\/>/', $svg, $bars, PREG_SET_ORDER);
    preg_match_all('/<circle cx="(\d+)" cy="\d+" r="6" fill="#FEDCBA"\/>/', $svg, $markers, PREG_SET_ORDER);
    preg_match_all('/<line x1="(\d+)" y1="\d+" x2="(\d+)" y2="\d+" stroke="#FEDCBA"/', $svg, $lines, PREG_SET_ORDER);
    preg_match('/<line x1="(\d+)"[^>]+stroke="#ABCDEF"/', $svg, $annotation);
    preg_match_all('/<text x="(\d+)"[^>]+>category\d+<\/text>/', $svg, $labels, PREG_SET_ORDER);
    preg_match_all('/<text x="(\d+)" y="16"[^>]+text-anchor="middle">1<\/text>/', $svg, $values, PREG_SET_ORDER);
    $ok = count($bars) === $count && count($markers) === $count && count($lines) === $count - 1;
    $ok = $ok && count($labels) === $count && count($values) === $count;
    foreach ($bars as $i => $bar) {
        // Independent integer boundaries: no accumulated truncated slot width.
        $left = 31 + intdiv($span * $i, $count);
        $right = 31 + intdiv($span * ($i + 1), $count);
        $gap = min(max(1, intdiv($right - $left, 8)), intdiv($right - $left - 1, 2));
        $x = (int)$bar[1];
        $width = (int)$bar[2];
        $ok = $ok && $x === $left + $gap && $width === $right - $left - 2 * $gap
            && $width > 0 && $x >= $left && $x + $width <= $right
            && (int)$markers[$i][1] === $x + $width;
        $ok = $ok && (int)$labels[$i][1] === $x + intdiv($width, 2)
            && (int)$values[$i][1] === $x + intdiv($width, 2);
        if ($i > 0) {
            $ok = $ok && $lines[$i - 1][1] === $markers[$i - 1][1]
                && $lines[$i - 1][2] === $markers[$i][1];
        }
    }
    $last = $bars[$count - 1];
    $center = (int)$last[1] + intdiv((int)$last[2], 2);
    $ok = $ok && abs($center - (int)$annotation[1]) <= 1;
    echo "$span/$count: ", $ok ? 'aligned' : 'misaligned', "\n";
    echo 'repeat: ', $chart->renderSvg() === $svg ? 'identical' : 'different', "\n";
}
foreach ([[399, 100], [400, 100], [128, 128], [129, 128], [256, 128],
          [257, 128], [127, 128], [320, 4], [399, 1]] as [$span, $count]) {
    checkSlots($span, $count);
}
?>
--EXPECT--
399/100: aligned
repeat: identical
400/100: aligned
repeat: identical
128/128: aligned
repeat: identical
129/128: aligned
repeat: identical
256/128: aligned
repeat: identical
257/128: aligned
repeat: identical
127/128: rejected
320/4: aligned
repeat: identical
399/1: aligned
repeat: identical
