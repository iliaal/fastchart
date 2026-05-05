--TEST--
renderPng / renderJpeg / renderWebp return encoded bytes without ext/gd ceremony
--EXTENSIONS--
fastchart
--FILE--
<?php

$base = (new FastChart\LineChart)
    ->setSize(400, 200)
    ->setTitle('quick render')
    ->setSeries([1, 4, 2, 8]);

// PNG
$png = $base->renderPng();
echo "png_sig: ",  substr(bin2hex($png), 0, 8) === '89504e47' ? "ok" : "bad", "\n";
echo "png_sane_size: ", (strlen($png) > 1024 && strlen($png) < 100000 ? "yes" : "no"), "\n";

// JPEG
$jpg = $base->renderJpeg();
echo "jpg_sig: ", substr(bin2hex($jpg), 0, 4) === 'ffd8' ? "ok" : "bad", "\n";

$jpg_low = $base->renderJpeg(20);
$jpg_hi  = $base->renderJpeg(95);
echo "jpg_low_smaller: ", (strlen($jpg_low) < strlen($jpg_hi) ? "yes" : "no"), "\n";

// WebP -- starts with "RIFF????WEBP"
$webp = $base->renderWebp();
echo "webp_riff: ", substr($webp, 0, 4) === 'RIFF' ? "ok" : "bad", "\n";
echo "webp_marker: ", substr($webp, 8, 4) === 'WEBP' ? "ok" : "bad", "\n";

// Quality bounds.
try {
    $base->renderJpeg(0);
    echo "jpg_q0: no throw\n";
} catch (\ValueError $e) {
    echo "jpg_q0: ValueError ok\n";
}

try {
    $base->renderWebp(101);
    echo "webp_q101: no throw\n";
} catch (\ValueError $e) {
    echo "webp_q101: ValueError ok\n";
}

// All five chart types produce PNG via the shortcut.
$rows = [];
for ($i = 0; $i < 8; $i++) {
    $rows[] = [1700000000 + $i * 86400, 100 + $i, 102 + $i, 99 + $i, 101 + $i, 1000];
}
$cases = [
    'line'    => (new FastChart\LineChart)->setSize(300, 200)->setSeries([1, 2, 3]),
    'bar'     => (new FastChart\BarChart)->setSize(300, 200)->setSeries([1, 5, 3]),
    'pie'     => (new FastChart\PieChart)->setSize(300, 300)->setSlices(['a' => 1, 'b' => 2]),
    'scatter' => (new FastChart\ScatterChart)->setSize(300, 200)->setPoints([[1, 1], [2, 3]]),
    'stock'   => (new FastChart\StockChart)->setSize(400, 250)->setOhlcv($rows),
];
foreach ($cases as $name => $c) {
    $b = $c->renderPng();
    echo "$name: ", substr(bin2hex($b), 0, 8) === '89504e47' ? "ok" : "bad", "\n";
}
?>
--EXPECT--
png_sig: ok
png_sane_size: yes
jpg_sig: ok
jpg_low_smaller: yes
webp_riff: ok
webp_marker: ok
jpg_q0: ValueError ok
webp_q101: ValueError ok
line: ok
bar: ok
pie: ok
scatter: ok
stock: ok
