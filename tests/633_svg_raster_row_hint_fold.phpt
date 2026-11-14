--TEST--
svgToPng: row hints are dropped when a one-row band folds its cells
--EXTENSIONS--
fastchart
--FILE--
<?php
/* A 600-vertex zigzag across a 4096x3 image puts more cells in a row than
 * the 64 KB pool holds, so one-row bands sort and fold their duplicate
 * cells inside gray_find_cell() (PV-23). A row hint left pointing into a
 * folded cell would walk the free list. Starting the same outline at
 * another vertex must render the same bytes. */
function doc(array $pts): string
{
    $d = '';
    foreach ($pts as $i => [$x, $y]) {
        $d .= ($i ? 'L' : 'M') . $x . ' ' . $y;
    }
    return '<svg xmlns="http://www.w3.org/2000/svg" width="4096" height="3">'
        . '<path fill="#000" d="' . $d . '"/></svg>';
}
foreach ([1, 2, 3, 4] as $seed) {
    mt_srand($seed);
    $p = [];
    for ($i = 0; $i < 600; $i++) {
        $p[] = [mt_rand(0, 4096 * 64 - 1) / 64, ($i & 1) ? 3 : 0];
    }
    $a = FastChart\Chart::svgToPng(doc($p));
    $b = FastChart\Chart::svgToPng(doc(array_merge(array_slice($p, 300), array_slice($p, 0, 300))));
    echo "seed $seed: ", $a === $b ? 'same' : 'differs', "\n";
}
?>
--EXPECT--
seed 1: same
seed 2: same
seed 3: same
seed 4: same
