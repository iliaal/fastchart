--TEST--
SunburstChart: nested, zero, and explicit parent values preserve wedge order
--EXTENSIONS--
fastchart
--FILE--
<?php
/* A recursive oracle follows the input tree, independent of the renderer's
 * flattened node representation. Check every polygon's angular endpoints. */
function value(array $node): float {
    return $node['value'] ?? array_sum(array_map('value', $node['children'] ?? []));
}
function wedges(array $node, int $depth, float $start, float $end, array &$expected): void {
    if ($depth) $expected[$depth][] = [$start, $end];
    $children = $node['children'] ?? [];
    $sum = array_sum(array_map('value', $children));
    $cursor = $start;
    foreach ($children as $child) {
        $a = $sum > 0 ? $cursor : 0.0;
        $b = $sum > 0 ? $cursor + ($end - $start) * (value($child) / $sum) : 0.0;
        wedges($child, $depth + 1, $a, $b, $expected);
        $cursor = $b;
    }
}
$tree = ['children' => [
    ['value' => 7, 'children' => [
        ['value' => 2],
        ['children' => [['value' => 1], ['value' => 3]]],
        ['value' => 0],
    ]],
    ['value' => 3, 'children' => [['value' => 0], ['value' => 0]]],
    ['value' => 0, 'children' => [['value' => 1], ['value' => 2]]],
    ['value' => 5],
]];
foreach ([$tree, ['children' => array_reverse($tree['children'])]] as $root) {
    $chart = (new FastChart\SunburstChart(400, 400))
        ->setPlotRect(20, 20, 380, 380)->setHierarchy($root);
    $svg = $chart->renderSvg();
    $expected = [];
    wedges($root, 0, -M_PI_2, -M_PI_2 + 2 * M_PI, $expected);
    krsort($expected);
    preg_match_all('/<polygon points="([^"]+)" fill="(?!none)/', $svg, $matches);
    $index = 0;
    $ok = count($matches[1]) === array_sum(array_map('count', $expected));
    $maxDepth = max(array_keys($expected));
    $inner = intdiv(180, $maxDepth + 2);
    $ring = (180 - $inner) / $maxDepth;
    foreach ($expected as $depth => $angles) {
        $radius = $inner + $ring * $depth;
        foreach ($angles as [$a, $b]) {
            $points = explode(' ', $matches[1][$index++]);
            $segments = max(1, (int)ceil(($b - $a) / (M_PI / 30)));
            foreach ([0 => $a, $segments => $b] as $at => $angle) {
                [$x, $y] = array_map('intval', explode(',', $points[$at]));
                /* Allow a one-pixel libm boundary difference across platforms. */
                $ok = $ok && abs($x - (200 + (int)($radius * cos($angle) + 0.5))) <= 1
                    && abs($y - (200 + (int)($radius * sin($angle) + 0.5))) <= 1;
            }
        }
    }
    echo 'all wedges in order: ', $ok ? 'yes' : 'no', "\n";
    echo 'repeat is identical: ', $chart->renderSvg() === $svg ? 'yes' : 'no', "\n";
}
?>
--EXPECT--
all wedges in order: yes
repeat is identical: yes
all wedges in order: yes
repeat is identical: yes
