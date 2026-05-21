<?php
/* Compare two perf.php JSON outputs and print per-scenario deltas. */

if ($argc < 3) {
    fwrite(STDERR, "usage: php diff.php <baseline.json> <candidate.json>\n");
    exit(1);
}
$base = json_decode(file_get_contents($argv[1]), true);
$cand = json_decode(file_get_contents($argv[2]), true);
if (!$base || !$cand) { fwrite(STDERR, "bad input\n"); exit(1); }

printf("baseline: %s    candidate: %s\n",
    $base['label'] ?? '?', $cand['label'] ?? '?');
printf("%-14s | %10s %10s | %10s %10s | %8s %8s | %s\n",
    'scenario', 'base p50', 'cand p50', 'base min', 'cand min', 'delta%', 'min%', 'out_size_ok');
echo str_repeat('-', 110), "\n";

foreach ($base['results'] as $name => $b) {
    $c = $cand['results'][$name] ?? null;
    if (!$c) continue;
    $d_p50 = ($c['p50'] - $b['p50']) / $b['p50'] * 100.0;
    $d_min = ($c['min'] - $b['min']) / $b['min'] * 100.0;
    $out_ok = $b['out_len'] === $c['out_len'] ? 'same' : "{$b['out_len']}→{$c['out_len']}";
    printf("%-14s | %10.2f %10.2f | %10.2f %10.2f | %+7.1f%% %+7.1f%% | %s\n",
        $name, $b['p50'], $c['p50'], $b['min'], $c['min'], $d_p50, $d_min, $out_ok);
}
