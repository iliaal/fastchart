--TEST--
CirclePacking places flat siblings exactly where a scan of every placed sibling does
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Reference: the spiral placement written out with a full collision scan
 * from sibling 0 at every step, counting every test against the 8M work
 * budget. The extension must draw the same circles, including past the
 * budget cut-off. The 700-value case (seed 145) reaches sibling 325 with
 * 7,999,798 units spent, so counting one extra unit per placed sibling
 * moves the cut-off one sibling earlier. Values are listed in descending order, so equal radii
 * leave the geometry independent of qsort's tie order. */
function reference(array $values, int $w, int $h): array {
    $r = [];
    foreach ($values as $v) $r[] = max(sqrt($v), 1.0);
    rsort($r);
    $n = count($r);
    $rad_step = max(array_sum($r) / $n * 0.05, 0.01);
    $x = array_fill(0, $n, 0.0);
    $y = array_fill(0, $n, 0.0);
    $work = 0;
    $cut = -1;
    for ($i = 1; $i < $n; $i++) {
        if ($work > 8000000) {
            if ($cut < 0) $cut = $i;
            $x[$i] = $rad_step * $i * cos($i * 2.39996322972865332);
            $y[$i] = $rad_step * $i * sin($i * 2.39996322972865332);
            continue;
        }
        for ($s = 0, $placed = false; $s < 200000 && !$placed; $s++) {
            $x[$i] = $rad_step * $s * cos($s * 2.39996322972865332);
            $y[$i] = $rad_step * $s * sin($s * 2.39996322972865332);
            $placed = true;
            for ($j = 0; $j < $i; $j++) {
                $work++;
                $dx = $x[$i] - $x[$j];
                $dy = $y[$i] - $y[$j];
                $need = $r[$i] + $r[$j];
                if ($dx * $dx + $dy * $dy < $need * $need - 0.001) {
                    $placed = false;
                    break;
                }
            }
        }
    }
    $minx = $x[0] - $r[0]; $maxx = $x[0] + $r[0];
    $miny = $y[0] - $r[0]; $maxy = $y[0] + $r[0];
    for ($i = 1; $i < $n; $i++) {
        $minx = min($minx, $x[$i] - $r[$i]); $maxx = max($maxx, $x[$i] + $r[$i]);
        $miny = min($miny, $y[$i] - $r[$i]); $maxy = max($maxy, $y[$i] + $r[$i]);
    }
    $ccx = ($minx + $maxx) / 2.0; $ccy = ($miny + $maxy) / 2.0;
    $R = 0.0;
    for ($i = 0; $i < $n; $i++) {
        $x[$i] -= $ccx; $y[$i] -= $ccy;
        $R = max($R, sqrt($x[$i] * $x[$i] + $y[$i] * $y[$i]) + $r[$i]);
    }
    $avail = min($w - 24, $h - 12 - 24);
    $scale = ($avail / 2.0 * 0.97) / ($R + 2.0);
    $cx = ($w / 2.0); $cy = (24 + $h - 12) / 2.0;
    $out = [sprintf('%d %d %d', (int)$cx, (int)$cy, (int)(($R + 2.0) * $scale))];
    for ($i = 0; $i < $n; $i++) {
        $out[] = sprintf('%d %d %d', (int)($cx + $x[$i] * $scale),
            (int)($cy + $y[$i] * $scale), (int)($r[$i] * $scale));
    }
    return [$out, $cut];
}

function drawn(array $values, int $w, int $h): array {
    $kids = [];
    foreach ($values as $v) $kids[] = ['value' => $v];
    $svg = (new FastChart\CirclePacking($w, $h))
        ->setHierarchy(['children' => $kids])->renderSvg();
    preg_match_all('/<circle cx="(-?\d+)" cy="(-?\d+)" r="(\d+)" fill="none"/', $svg, $m, PREG_SET_ORDER);
    return array_map(fn($c) => "$c[1] $c[2] $c[3]", $m);
}

mt_srand(634);
$cases = [
    'distinct 60' => array_map(fn($i) => 1000 - $i * 13, range(0, 59)),
    'ties 120' => array_map(fn($i) => [400, 400, 90, 90, 90, 25][$i % 6], range(0, 119)),
    'random 250' => (function () { $v = []; for ($i = 0; $i < 250; $i++) $v[] = mt_rand(1, 5000); rsort($v); return $v; })(),
    'budget 700' => (function () { mt_srand(145); $v = []; for ($i = 0; $i < 700; $i++) $v[] = mt_rand(1, 1000); rsort($v); return $v; })(),
];
foreach ($cases as $name => $values) {
    [$want, $cut] = reference($values, 800, 800);
    $got = drawn($values, 800, 800);
    echo $name, ': ', $got === $want ? 'same' : 'DIFF', $cut >= 0 ? ", budget cut at $cut" : '', "\n";
}
?>
--EXPECTF--
distinct 60: same
ties 120: same
random 250: same
budget 700: same, budget cut at %d
