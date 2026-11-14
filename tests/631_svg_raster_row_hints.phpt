--TEST--
raster: row-hinted cell lookup keeps coverage, the work budget and tall bands
--EXTENSIONS--
fastchart
--FILE--
<?php

function cpu_seconds(): float
{
    $r = getrusage();
    return $r['ru_utime.tv_sec'] + $r['ru_utime.tv_usec'] / 1e6
        + $r['ru_stime.tv_sec'] + $r['ru_stime.tv_usec'] / 1e6;
}

function svg(int $w, int $h, string $body): string
{
    return '<svg xmlns="http://www.w3.org/2000/svg" width="' . $w
        . '" height="' . $h . '">' . $body . '</svg>';
}

/* One-pixel stripes, one subpath each, emitted left to right or right
 * to left. */
function stripes(int $w, int $h, bool $rtl): array
{
    $s = [];
    for ($k = 0; $k < intdiv($w, 2); $k++) {
        $x = $rtl ? $w - 2 - 2 * $k : 2 * $k;
        $s[] = 'M' . $x . ' 0h1V' . $h . 'h-1z';
    }
    return $s;
}

function polygon(array $pts): string
{
    $d = [];
    foreach ($pts as $i => [$x, $y]) {
        $d[] = ($i ? 'L' : 'M') . $x . ' ' . $y;
    }
    return implode('', $d) . 'Z';
}

/* Coverage is an exact integer sum per cell, so the order in which the
 * edges reach a row must not change a single byte. A 2,048-point noise
 * polygon crosses each row hundreds of times; started from other
 * vertices it reaches each row's cells in other orders, through hinted
 * starts, capped walks and the sort and fold. */
mt_srand(8);
$pts = [[0, 300]];
for ($i = 1; $i <= 2048; $i++) {
    $pts[] = [round(1200 * $i / 2048, 2), mt_rand(0, 600)];
}
foreach (['nonzero', 'evenodd'] as $rule) {
    $png = [];
    foreach ([0, 517, 1000, 1731] as $start) {
        $p = array_merge(array_slice($pts, $start), array_slice($pts, 0, $start));
        $png[] = FastChart\Chart::svgToPng(svg(1200, 600,
            '<path fill="#0a6" fill-rule="' . $rule . '" d="'
            . polygon($p) . '"/>'));
    }
    echo "noise $rule: ", count(array_unique($png)) === 1 ? "same" : "differs", "\n";
}

/* Stripes drawn left to right reach each row's cells in ascending x, the
 * order the hints serve without a walk; right to left they land at the
 * row head; as separate 5-point paths they never use hints. */
$ltr = FastChart\Chart::svgToPng(svg(512, 256,
    '<path fill="#369" d="' . implode('', stripes(512, 256, false)) . '"/>'));
$rtl = FastChart\Chart::svgToPng(svg(512, 256,
    '<path fill="#369" d="' . implode('', stripes(512, 256, true)) . '"/>'));
$sep = FastChart\Chart::svgToPng(svg(512, 256, implode('', array_map(
    fn($d) => '<path fill="#369" d="' . $d . '"/>', stripes(512, 256, false)))));
echo "stripes: ", ($ltr === $rtl && $rtl === $sep) ? "same" : "differs", "\n";

/* A 64-segment zigzag one pixel tall puts 4,096 cells in row 0, more
 * than the 32 KB and 64 KB pools hold, so the render is retried in a
 * larger heap pool whose hint array (and taller bands) come with it. */
$wide = [];
for ($k = 0; $k <= 64; $k++) {
    $wide[] = [min(4095, 64 * $k), ($k & 1) ? 1 : 0];
}
$wide[] = [4095, 4096];
$wide[] = [0, 4096];
$png = [];
foreach ([0, 20, 40, 66] as $start) {
    $p = array_merge(array_slice($wide, $start), array_slice($wide, 0, $start));
    $png[] = FastChart\Chart::svgToPng(svg(4096, 4096,
        '<path fill="#0a6" d="' . polygon($p) . '"/>'));
}
echo "wide row: ", count(array_unique($png)) === 1 ? "same" : "differs", "\n";

/* Charts are not capped at 4096 px. In a 400x8000 line or area chart the
 * plot outline (64+ points, smallest pool) needs more than the 39 bands
 * the rasterizer allows, so the last band is taller than the hint array
 * (two links per row for twice the band size) and walks from the row head;
 * the bands it is halved into use hints again. */
mt_srand(2);
$tall = [];
for ($i = 0; $i < 100; $i++) {
    $tall[] = mt_rand(0, 1000);
}
foreach (['line' => 'FastChart\\LineChart', 'area' => 'FastChart\\AreaChart'] as $kind => $cls) {
    $a = (new $cls(400, 8000))->setSeries([['data' => $tall]])->renderPng();
    $b = (new $cls(400, 8000))->setSeries([['data' => $tall]])->renderPng();
    $ok = strncmp($a, "\x89PNG\r\n\x1a\n", 8) === 0 && $a === $b
        && getimagesizefromstring($a)[1] === 8000;
    echo "tall $kind: ", $ok ? "renders" : "differs", "\n";
}

/* Budget. Left-to-right stripes used to walk 32 cells per new cell before
 * giving up, so two full-height 4096 px paths cost ~3.58G units and were
 * refused; with hints they cost what right-to-left stripes cost (~1.10G
 * vs ~1.06G) and render. Eight such paths (~4.39G) are still refused. */
$t = cpu_seconds();
try {
    $ltr2 = FastChart\Chart::svgToPng(svg(4096, 4096, str_repeat(
        '<path fill="#369" d="' . implode('', stripes(4096, 4096, false)) . '"/>', 2)));
    $rtl2 = FastChart\Chart::svgToPng(svg(4096, 4096, str_repeat(
        '<path fill="#369" d="' . implode('', stripes(4096, 4096, true)) . '"/>', 2)));
    echo "ltr x2: ", $ltr2 === $rtl2 ? "renders as rtl" : "differs", "\n";
} catch (ValueError $e) {
    echo "ltr x2: ", $e->getMessage(), "\n";
}
try {
    FastChart\Chart::svgToPng(svg(4096, 4096, str_repeat(
        '<path fill="#369" d="' . implode('', stripes(4096, 4096, false)) . '"/>', 8)));
    echo "ltr x8: rendered\n";
} catch (ValueError $e) {
    echo "ltr x8: ", $e->getMessage(), "\n";
}
$t = cpu_seconds() - $t;
echo "cpu: ", ($t < 120.0 ? "bounded" : sprintf("%.1fs", $t)), "\n";
?>
--EXPECT--
noise nonzero: same
noise evenodd: same
stripes: same
wide row: same
tall line: renders
tall area: renders
ltr x2: renders as rtl
ltr x8: FastChart\Chart::svgToPng() SVG render work exceeds cap (fill coverage too large)
cpu: bounded
