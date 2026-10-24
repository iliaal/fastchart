--TEST--
SunburstChart setter aggregates direct children across reallocations and preserves error precedence
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Annotate only implicit values, retaining the input-order additions used by
 * the native setter. Explicit values need not equal their children's sum. */
function explicitValues(array $node): array {
    $sum = 0.0;
    if (isset($node['children'])) {
        foreach ($node['children'] as &$child) {
            if (!is_array($child)) continue;
            $child = explicitValues($child);
            $sum += $child['value'];
        }
        unset($child);
    }
    $node['value'] ??= $sum;
    return $node;
}
function chain(int $depth): array {
    $node = ['value' => 1.0];
    for ($i = 0; $i < $depth; $i++) $node = ['children' => [$node]];
    return $node;
}
$children = [];
for ($i = 0; $i < 40; $i++) {
    $children[] = ['children' => [
        ['value' => $i + 1],
        ['value' => 0.0, 'children' => [['value' => 2.0], ['value' => 3.0]]],
        false,
        ['children' => [['value' => 1.0e-100], ['value' => 2.0e-100]]],
    ]];
}
$cases = [
    'reallocated' => ['children' => $children],
    'depth limit' => chain(32),
    'node limit' => ['children' => array_fill(0, 2047, ['value' => 1.0])],
];
foreach ($cases as $name => $root) {
    $chart = (new FastChart\SunburstChart(400, 400))->setHierarchy($root);
    $expected = (new FastChart\SunburstChart(400, 400))->setHierarchy(explicitValues($root));
    $svg = $chart->renderSvg();
    echo $name, ': ', $svg === $expected->renderSvg() && $svg === (clone $chart)->renderSvg() ? 'same' : 'BAD', "\n";
}
$before = $chart->renderSvg();
foreach ([
    'depth before overflow' => ['children' => [['value' => 1e308], ['value' => 1e308], chain(32)]],
    'explicit parent overflow' => ['value' => 0.0, 'children' => [['value' => 1e308], ['value' => 1e308]]],
] as $name => $root) {
    try {
        $chart->setHierarchy($root);
        echo "$name: accepted\n";
    } catch (ValueError $e) {
        echo $name, ': ', str_contains($e->getMessage(), 'malformed hierarchy') ? 'structural' : 'numeric', "\n";
    }
    echo 'state: ', $before === $chart->renderSvg() ? 'preserved' : 'BAD', "\n";
}
?>
--EXPECT--
reallocated: same
depth limit: same
node limit: same
depth before overflow: structural
state: preserved
explicit parent overflow: numeric
state: preserved
