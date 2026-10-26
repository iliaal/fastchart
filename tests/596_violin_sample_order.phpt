--TEST--
ViolinPlot subsampling preserves both modes of periodic samples
--EXTENSIONS--
fastchart
--FILE--
<?php
function chart(array $values): FastChart\ViolinPlot {
    return (new FastChart\ViolinPlot(400, 300))->setGroups([
        ['color' => 0x123456, 'values' => $values],
    ]);
}
/* Exercise both sides of the stride transitions, plus the input limit. */
foreach ([1023, 1024, 1025, 2047, 2048, 2049, 8192] as $n) {
    $values = [];
    for ($i = 0; $i < $n; $i++) $values[] = $i % 2 ? 100 : 0;
    $original = chart($values);
    $svg = $original->renderSvg();
    sort($values);
    echo "$n sorted: ", $svg === chart($values)->renderSvg() ? 'same' : 'BAD', "\n";
    echo "$n reverse: ", $svg === chart(array_reverse($values))->renderSvg() ? 'same' : 'BAD', "\n";
    echo "$n clone/repeat: ", $svg === (clone $original)->renderSvg()
        && $svg === $original->renderSvg() ? 'same' : 'BAD', "\n";
    /* The complete silhouette must retain both equally populated modes,
     * not merely produce the same (possibly collapsed) result. */
    preg_match('/<polygon[^>]*points="([^"]+)"[^>]*fill="#123456"/', $svg, $m);
    $points = preg_split('/\s+/', trim($m[1] ?? ''));
    $xy = array_map(fn($p) => array_map('intval', explode(',', $p)), $points);
    $last = count($xy) - 1;
    $bottomWidth = ($xy[0][0] ?? 0) - ($xy[$last][0] ?? 0);
    $topWidth = ($xy[47][0] ?? 0) - ($xy[48][0] ?? 0);
    echo "$n both modes: ", count($xy) === 96 && $bottomWidth > 250 && $topWidth > 250 ? 'yes' : 'BAD', "\n";
}
?>
--EXPECT--
1023 sorted: same
1023 reverse: same
1023 clone/repeat: same
1023 both modes: yes
1024 sorted: same
1024 reverse: same
1024 clone/repeat: same
1024 both modes: yes
1025 sorted: same
1025 reverse: same
1025 clone/repeat: same
1025 both modes: yes
2047 sorted: same
2047 reverse: same
2047 clone/repeat: same
2047 both modes: yes
2048 sorted: same
2048 reverse: same
2048 clone/repeat: same
2048 both modes: yes
2049 sorted: same
2049 reverse: same
2049 clone/repeat: same
2049 both modes: yes
8192 sorted: same
8192 reverse: same
8192 clone/repeat: same
8192 both modes: yes
