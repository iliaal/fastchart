--TEST--
Degenerate gradient geometry paints without float-to-int overflow
--DESCRIPTION--
A focal point far outside a huge radial gradient yields NaN gradient
positions. A sanitizer build with float-cast-overflow aborts on the
conversion under these UBSAN_OPTIONS; otherwise the pixel checks run.
--EXTENSIONS--
fastchart
gd
--ENV--
UBSAN_OPTIONS=halt_on_error=1:abort_on_error=1:print_stacktrace=1
--INI--
asan.detect_leaks=0
--FILE--
<?php

function paint(string $gradient)
{
    $png = FastChart\Chart::svgToPng(
        '<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64"><defs>'
        . $gradient . '</defs><rect width="64" height="64" fill="url(#g)"/></svg>');
    return imagecreatefromstring($png);
}

$stops = '<stop offset="0" stop-color="red"/><stop offset="1" stop-color="blue"/>';

$far = paint('<radialGradient id="g" gradientUnits="userSpaceOnUse" cx="1e30" '
    . 'cy="32" r="1e30" fx="-1e30" fy="32">' . $stops . '</radialGradient>');
echo 'far focal point: ', $far ? "painted\n" : "NO\n";

$wide = paint('<radialGradient id="g" gradientUnits="userSpaceOnUse" cx="32" '
    . 'cy="32" r="1e38" fx="32" fy="32" fr="3e38">' . $stops . '</radialGradient>');
echo 'focal radius past FLT_MAX: ', $wide ? "painted\n" : "NO\n";

$flat = paint('<linearGradient id="g" gradientUnits="userSpaceOnUse" x1="0" '
    . 'y1="0" x2="1e-30" y2="1e30" spreadMethod="repeat">' . $stops
    . '</linearGradient>');
echo 'near-zero linear vector: ', $flat ? "painted\n" : "NO\n";

$normal = paint('<linearGradient id="g" x1="0" y1="0" x2="1" y2="0">'
    . $stops . '</linearGradient>');
$left = imagecolorat($normal, 1, 32);
$right = imagecolorat($normal, 62, 32);
echo 'ordinary gradient: ',
    ((($left >> 16) & 0xFF) > 200 && (($right & 0xFF) > 200)) ? "red to blue\n" : "NO\n";

?>
--EXPECT--
far focal point: painted
focal radius past FLT_MAX: painted
near-zero linear vector: painted
ordinary gradient: red to blue
