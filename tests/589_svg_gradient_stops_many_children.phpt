--TEST--
svgToPng: a gradient with many non-stop children resolves its stops once per paint, not per child
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

function render_cpu(int $padding): float
{
    $rects = [];
    for ($i = 0; $i < 16000; $i++) {
        $rects[] = '<rect x="' . ($i % 256) . '" y="' . (($i >> 8) % 256)
            . '" width="1" height="1" fill="url(#g)" stroke="url(#g)"/>';
    }
    $svg = '<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256">'
        . '<defs><linearGradient id="g">' . str_repeat('<g/>', $padding)
        . '<stop offset="0" stop-color="#f00"/>'
        . '<stop offset="1" stop-color="#00f"/>'
        . '</linearGradient></defs>' . implode('', $rects) . '</svg>';
    $t = cpu_seconds();
    $png = FastChart\Chart::svgToPng($svg);
    $t = cpu_seconds() - $t;
    $info = getimagesizefromstring($png);
    echo "padding $padding: {$info[0]}x{$info[1]}\n";
    return $t;
}

/* 16,000 rects paint their fill and stroke with one gradient. Padding the
 * gradient with 16,000 <g/> children before its stops used to cost every
 * paint a walk over all of them, 0.5G child visits (7.4 s against 0.56 s
 * unpadded on a release build); the stops are now linked at parse time,
 * so the padding adds only its parse. */
$plain = render_cpu(0);
$padded = render_cpu(16000);
echo "padding cost: ",
    ($padded < 2 * $plain + 0.5 ? "bounded"
        : sprintf("%.2fs vs %.2fs", $padded, $plain)), "\n";
?>
--EXPECT--
padding 0: 256x256
padding 16000: 256x256
padding cost: bounded
