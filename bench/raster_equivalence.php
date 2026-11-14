<?php
/* Raster differential harness: one line per render, "<name>\t<md5 of the
 * output>" or "<name>\trefused: <message>". Run it against two builds and
 * diff the results:
 *
 *   php -d extension=/path/base.so  bench/raster_equivalence.php > base.txt
 *   php -d extension=/path/head.so  bench/raster_equivalence.php > head.txt
 *   diff base.txt head.txt
 *
 * A raster change that must keep coverage byte-identical should show no
 * differences except documents whose svgTo*() render-work charge crossed the
 * 2.2G-unit cap; for those, check that the newly rendered output is the one
 * an unbudgeted build produces. Covers dense line/area/scatter charts at
 * random sizes, random SVG paths (polygons, right-to-left noise, scattered
 * boxes, strided zigzags, mirrored sweeps, curves; fills under both rules
 * and strokes), and the stripe and zigzag documents the raster work tests
 * use. Takes about a minute with a release build. Pass a seed count to
 * scale it: `php bench/raster_equivalence.php 400`. */

if (!extension_loaded('fastchart')) {
    fwrite(STDERR, "fastchart extension not loaded\n");
    exit(1);
}
$N = (int)($argv[1] ?? 120);

function out(string $name, callable $f): void {
    try {
        $o = md5($f());
    } catch (Throwable $e) {
        $o = 'refused: ' . $e->getMessage();
    }
    echo $name, "\t", $o, "\n";
}
function rf(float $a, float $b): float { return $a + ($b - $a) * mt_rand() / mt_getrandmax(); }
function doc(int $w, int $h, string $body): string {
    return '<svg xmlns="http://www.w3.org/2000/svg" width="' . $w . '" height="' . $h . '">' . $body . '</svg>';
}

/* dense charts */
for ($k = 0; $k < $N; $k++) {
    mt_srand(5000 + $k);
    $w = [320, 640, 800, 1200, 1920][mt_rand(0, 4)];
    $h = [200, 400, 600, 1080][mt_rand(0, 3)];
    $ns = mt_rand(1, 6);
    $n = [20, 200, 1000, 2048][mt_rand(0, 3)];
    $kind = mt_rand(0, 3);
    $ser = [];
    for ($s = 0; $s < $ns; $s++) {
        $a = [];
        $v = 500;
        for ($i = 0; $i < $n; $i++) {
            $v = match ($kind) {
                0 => mt_rand(0, 1000),
                1 => $v + mt_rand(-10, 10),
                2 => 500 + 300 * sin($i / 40 + $s) + mt_rand(-60, 60),
                3 => abs(sin($i * 0.37)) * 900 + mt_rand(0, 40),
            };
            $a[] = $v;
        }
        $ser[] = ['data' => $a];
    }
    $type = mt_rand(0, 3);
    $name = "chart$k t$type k$kind {$ns}x$n {$w}x$h";
    if ($type == 0) {
        out($name, fn() => (new FastChart\LineChart($w, $h))->setSeries($ser)->renderPng());
    } elseif ($type == 1) {
        out($name, fn() => (new FastChart\AreaChart($w, $h))->setSeries($ser)->renderPng());
    } elseif ($type == 2) {
        out($name, fn() => (new FastChart\AreaChart($w, $h))->setSeries($ser)->setStacked(true)->renderPng());
    } else {
        $pts = [];
        foreach ($ser[0]['data'] as $i => $v) {
            $pts[] = [$i, $v];
        }
        out($name, fn() => (new FastChart\ScatterChart())->setSize($w, $h)->setPoints($pts)->renderPng());
    }
}

/* random SVG paths */
for ($k = 0; $k < $N; $k++) {
    mt_srand(90000 + $k);
    $W = [256, 1024, 2048][mt_rand(0, 2)];
    $H = [64, 512, 1024][mt_rand(0, 2)];
    $mode = mt_rand(0, 5);
    $n = [200, 1000, 3000][mt_rand(0, 2)];
    $d = 'M' . rf(0, $W) . ' ' . rf(0, $H);
    for ($i = 0; $i < $n; $i++) {
        $d .= match ($mode) {
            0 => 'L' . rf(0, $W) . ' ' . rf(0, $H),
            1 => 'L' . ($W - $W * $i / $n) . ' ' . rf(0, $H),
            2 => 'M' . rf(0, $W) . ' ' . rf(0, $H) . 'h' . rf(1, 40) . 'v' . rf(0.2, 30) . 'h-' . rf(1, 40) . 'z',
            3 => 'L' . (($i * 37) % $W) . ' ' . (($i & 1) ? $H : 0),
            4 => 'L' . ($i & 1 ? $W - ($i % $W) : $i % $W) . ' ' . rf(0, $H),
            5 => 'Q' . rf(0, $W) . ' ' . rf(0, $H) . ' ' . rf(0, $W) . ' ' . rf(0, $H),
        };
    }
    $paint = mt_rand(0, 1)
        ? ' fill="none" stroke="#000" stroke-width="' . rf(0.5, 6) . '"'
        : (mt_rand(0, 1) ? ' fill-rule="evenodd"' : '');
    $svg = doc($W, $H, '<path d="' . $d . '"' . $paint . '/>');
    out("path$k m$mode n$n {$W}x$H", fn() => FastChart\Chart::svgToPng($svg));
}

/* one-pixel stripes in both directions and full-height zigzags */
foreach ([1, 2] as $rep) {
    foreach ([false, true] as $rtl) {
        $s = '';
        for ($i = 0; $i <= 2047; $i++) {
            $x = $rtl ? 4094 - 2 * $i : 2 * $i;
            $s .= 'M' . $x . ' 0h1V4096h-1z';
        }
        $svg = doc(4096, 4096, str_repeat('<path fill="#369" d="' . $s . '"/>', $rep));
        out('stripes ' . ($rtl ? 'rtl' : 'ltr') . " x$rep", fn() => FastChart\Chart::svgToPng($svg));
    }
}
foreach ([1000, 4000] as $n) {
    $d = 'M0 0';
    for ($i = 0; $i < $n; $i++) {
        $d .= 'L' . ($i % 4096) . ' ' . (($i & 1) ? 4096 : 0);
    }
    $svg = doc(4096, 4096, '<path d="' . $d . '"/>');
    out("zigzag fill $n", fn() => FastChart\Chart::svgToPng($svg));
}
