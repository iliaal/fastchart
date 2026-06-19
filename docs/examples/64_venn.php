<?php
/* VennDiagram: 2 or 3 sets with circle areas proportional to set size
 * and centre distances solved so each overlap lens area matches the
 * requested intersection. Translucent fills blend at the overlaps.
 * Here, candidate skill coverage across three roles. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\VennDiagram(560, 480))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Skill overlap across roles')
    ->setSets([
        ['label' => 'Backend',  'size' => 120, 'color' => 0xCC4444],
        ['label' => 'Frontend', 'size' => 100, 'color' => 0x4477CC],
        ['label' => 'DevOps',   'size' => 80,  'color' => 0x44AA88],
    ])
    ->setIntersections([
        ['sets' => [0, 1], 'size' => 30],
        ['sets' => [0, 2], 'size' => 28],
        ['sets' => [1, 2], 'size' => 18],
    ])
    ->renderToFile(__DIR__ . '/64_venn.png');
