<?php
/* ViolinPlot: a gaussian kernel-density estimate of each group's
 * distribution, mirrored about its centre line, with the median marked.
 * Shows the shape (modes, skew, spread) a box plot's five-number
 * summary hides. Here, request-latency distributions for three
 * services. Samples are generated deterministically (Box-Muller on a
 * fixed seed) so the rendered output is stable. */

require __DIR__ . '/_bootstrap.php';

mt_srand(20260618);
function fc_samples(float $mu, float $sd, int $n): array {
    $v = [];
    for ($i = 0; $i < $n; $i++) {
        $u1 = (mt_rand() + 1) / (mt_getrandmax() + 1);
        $u2 = (mt_rand() + 1) / (mt_getrandmax() + 1);
        $v[] = $mu + $sd * sqrt(-2 * log($u1)) * cos(2 * M_PI * $u2);
    }
    return $v;
}

(new FastChart\ViolinPlot(680, 420))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Request latency by service (ms)')
    ->setGroups([
        ['label' => 'auth',    'color' => 0x44AA88, 'values' => fc_samples(48, 9, 240)],
        ['label' => 'catalog', 'color' => 0x6C8EBF, 'values' => fc_samples(72, 6, 240)],
        ['label' => 'search',  'color' => 0xD79B00, 'values' => fc_samples(60, 16, 200)],
    ])
    ->renderToFile(__DIR__ . '/61_violin.png');
