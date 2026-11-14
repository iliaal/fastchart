--TEST--
svgToPng: cell-list walk steps still count toward the raster work budget
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

/* 31 full-height one-pixel stripes, repeated in one path, each pass in
 * the order k * 11 mod 31 shifted by the pass. Every row ends up with 62
 * cells that the stripes keep revisiting in an order neither of the
 * row's last two cells serves, so most lookups walk up to 31 cells from
 * the row head, short of the 32-step cap. */
function shuffled_stripes(int $passes): string
{
    $d = '';
    for ($r = 0; $r < $passes; $r++) {
        for ($k = 0; $k < 31; $k++) {
            $d .= 'M' . (2 * (($k * 11 + $r) % 31)) . ' 0h1V4096h-1z';
        }
    }
    return '<svg xmlns="http://www.w3.org/2000/svg" width="62" height="4096">'
        . '<path fill="#369" d="' . $d . '"/></svg>';
}

/* 250 passes cost ~1.46G work units and render; 500 passes cost ~2.91G,
 * ~2.15G of them walk steps, and are refused. Without the walk charge
 * 500 passes would cost ~0.76G and render. Release: ~2.4 s to render 250,
 * ~4 s to refuse 500. */
$t = cpu_seconds();
foreach ([250, 500] as $passes) {
    try {
        $png = FastChart\Chart::svgToPng(shuffled_stripes($passes));
        echo "$passes: ", getimagesizefromstring($png)[1], "\n";
    } catch (ValueError $e) {
        echo "$passes: ", $e->getMessage(), "\n";
    }
}
$t = cpu_seconds() - $t;
echo "cpu: ", ($t < 120.0 ? "bounded" : sprintf("%.1fs", $t)), "\n";
?>
--EXPECT--
250: 4096
500: FastChart\Chart::svgToPng() SVG render work exceeds cap (fill coverage too large)
cpu: bounded
