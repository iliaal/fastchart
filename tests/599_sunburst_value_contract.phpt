--TEST--
SunburstChart documented values match explicit, fallback, and sibling-weight behavior
--EXTENSIONS--
fastchart
--FILE--
<?php
function check(bool $ok, string $name): void {
    if (!$ok) throw new RuntimeException($name);
}
function render(array $root): string {
    return (new FastChart\SunburstChart(400, 400))->setHierarchy($root)->renderSvg();
}
function compactText(string $text): string {
    return trim(preg_replace('/\s+/', ' ', preg_replace('/^\s*\* ?/m', '', $text)));
}
$stub = file_get_contents(__DIR__ . '/../fastchart.stub.php');
preg_match('~/\*\*\s*\* Sunburst \(.*?\*/~s', $stub, $class);
$classClaim = "Sunburst (radial hierarchical donut): nested rings where each ring is a level of the hierarchy. Children divide their parent's angular span in proportion to their values relative to their siblings, not to an explicit parent value. Zero-total siblings have zero-width wedges.";
check(str_contains(compactText($class[0]), $classClaim), 'complete class documentation');
preg_match('~final class SunburstChart.*?/\*\*(.*?)\*/~s', $stub, $method);
$methodClaim = "Hierarchical tree. Each node: `['label' => string?, 'value' => number?, 'color' => int?, 'children' => array?]`. A finite, non-negative int or float (including zero) is an explicit value, even for an interior node; it need not equal the sum of its children. Missing or invalid values (including numeric strings, booleans, null, negatives, NaN and infinity) fall back to the sum of direct children's effective values, or zero for a leaf. Non-array children are ignored. Rendering requires a positive effective root value. Overflowing child totals are rejected even when their parent has an explicit value.";
check(compactText($method[1]) === $methodClaim, 'complete setter documentation');
$guide = compactText(file_get_contents(__DIR__ . '/../docs/README.md'));
check(str_contains($guide, "`FastChart\\SunburstChart` is a nested ring donut. Each ring is one level of the hierarchy. Children divide their parent's angular span in proportion to their values relative to their siblings, not to an explicit parent value. Zero-total siblings have zero-width wedges."), 'guide geometry');
check(str_contains($guide, "A finite, non-negative integer or float (including zero) is an explicit `value`, even for an interior node; it need not equal the sum of its children. Missing or invalid values (including numeric strings, booleans, null, negatives, NaN and infinity) fall back to the sum of direct children's effective values, or zero for a leaf. Non-array children are ignored. Rendering requires a positive effective root value. Overflowing child totals are rejected even when their parent has an explicit value."), 'guide value contract');
echo "complete documentation: ok\n";
$branch = ['children' => [['value' => 1], ['value' => 3]]];
$root = ['children' => [$branch, ['value' => 4]]];
$baseline = render($root);
foreach ([null, false, true, '9', '', [], new stdClass, -1, -0.5, NAN, INF, -INF] as $invalid) {
    $candidate = $root;
    $candidate['children'][0]['value'] = $invalid;
    check(render($candidate) === $baseline, 'invalid interior falls back');
    check(render(['children' => [['value' => 4], ['value' => $invalid]]]) === render(['children' => [['value' => 4], ['value' => 0]]]), 'invalid leaf is zero');
}
check(render(['children' => [['value' => 4], []]]) === render(['children' => [['value' => 4], ['value' => 0]]]), 'missing leaf is zero');
check(render(['children' => [$branch, false, 'ignored', ['value' => 4]]]) === $baseline, 'non-array children ignored');
foreach ([null, false, 'invalid'] as $children) {
    check(render(['children' => [['value' => 4], ['children' => $children]]]) === render(['children' => [['value' => 4], ['value' => 0]]]), 'non-array child list is leaf');
}
echo "fallback values: ok\n";
foreach ([0, 0.0, 2, 2.5] as $value) {
    $candidate = $root;
    $candidate['children'][0]['value'] = $value;
    $scaledChildren = $candidate;
    $scaledChildren['children'][0]['children'] = [['value' => 2], ['value' => 6]];
    check(render($candidate) === render($scaledChildren), 'explicit parent independent of child total');
    check(render($candidate) !== $baseline, 'explicit parent overrides sum');
}
$reference = 2.5;
$referenced = $root;
$referenced['children'][0]['value'] =& $reference;
$explicit = $root;
$explicit['children'][0]['value'] = 2.5;
check(render($referenced) === render($explicit), 'referenced explicit value');
foreach ([1, 999.0] as $value) {
    check(render(['value' => $value] + $root) === $baseline, 'root value does not divide children');
}
echo "explicit sibling weights: ok\n";
foreach ([[], ['value' => 0] + $root, ['children' => [['value' => 0]]]] as $zero) {
    try { render($zero); throw new RuntimeException('zero root rendered'); }
    catch (Error $e) { check(str_contains($e->getMessage(), 'positive values'), 'zero root error'); }
}
try {
    (new FastChart\SunburstChart)->setHierarchy(['value' => 1, 'children' => [['value' => PHP_FLOAT_MAX], ['value' => PHP_FLOAT_MAX]]]);
    throw new RuntimeException('overflow accepted');
} catch (ValueError $e) {
    check(str_contains($e->getMessage(), 'aggregate values'), 'explicit root overflow');
}
echo "root and overflow guards: ok\n";
?>
--EXPECT--
complete documentation: ok
fallback values: ok
explicit sibling weights: ok
root and overflow guards: ok
