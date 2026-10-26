<?php
/* Compare two perf.php JSON outputs and print per-scenario deltas. */

if ($argc < 3) {
    fwrite(STDERR, "usage: php diff.php <baseline.json> <candidate.json>\n");
    exit(1);
}
/* Validate both complete reports before printing any comparison rows. */
function readReport(string $path): array {
    $json = @file_get_contents($path);
    if ($json === false) throw new RuntimeException("Cannot read report: $path");
    try {
        $report = json_decode($json, true, 512, JSON_THROW_ON_ERROR);
    } catch (JsonException $e) {
        throw new RuntimeException("Invalid JSON report: $path");
    }
    if (!is_array($report) || !isset($report['results'])
        || !is_array($report['results']) || !$report['results']) {
        throw new RuntimeException("Report must contain nonempty results: $path");
    }
    if (isset($report['label']) && !is_string($report['label'])) {
        throw new RuntimeException("Report label must be a string: $path");
    }
    foreach ($report['results'] as $name => $row) {
        if (!is_string($name) || !is_array($row)) {
            throw new RuntimeException("Invalid scenario in report: $path");
        }
        foreach (['p50', 'min'] as $metric) {
            $value = $row[$metric] ?? null;
            if ((!is_int($value) && !is_float($value)) || !is_finite((float)$value) || $value < 0) {
                throw new RuntimeException("Invalid $metric for scenario $name: $path");
            }
        }
        if (!isset($row['out_len']) || !is_int($row['out_len']) || $row['out_len'] < 0) {
            throw new RuntimeException("Invalid out_len for scenario $name: $path");
        }
    }
    return $report;
}

try {
    $base = readReport($argv[1]);
    $cand = readReport($argv[2]);
    if (array_diff_key($base['results'], $cand['results'])
        || array_diff_key($cand['results'], $base['results'])) {
        throw new RuntimeException('Reports must contain the same scenarios');
    }
} catch (RuntimeException $e) {
    fwrite(STDERR, $e->getMessage() . "\n");
    exit(1);
}

/* A zero baseline has no defined relative percentage, even for zero -> zero. */
function percentChange(float $base, float $candidate): string {
    if ($base == 0.0) return 'n/a';
    $change = ($candidate - $base) / $base * 100.0;
    return is_finite($change) ? sprintf('%+7.1f%%', $change) : 'n/a';
}

printf("baseline: %s    candidate: %s\n",
    $base['label'] ?? '?', $cand['label'] ?? '?');
printf("%-14s | %10s %10s | %10s %10s | %8s %8s | %s\n",
    'scenario', 'base p50', 'cand p50', 'base min', 'cand min', 'delta%', 'min%', 'out_size_ok');
echo str_repeat('-', 110), "\n";

foreach ($base['results'] as $name => $b) {
    $c = $cand['results'][$name];
    $d_p50 = percentChange($b['p50'], $c['p50']);
    $d_min = percentChange($b['min'], $c['min']);
    $out_ok = $b['out_len'] === $c['out_len'] ? 'same' : "{$b['out_len']}→{$c['out_len']}";
    printf("%-14s | %10.2f %10.2f | %10.2f %10.2f | %8s %8s | %s\n",
        $name, $b['p50'], $c['p50'], $b['min'], $c['min'], $d_p50, $d_min, $out_ok);
}
