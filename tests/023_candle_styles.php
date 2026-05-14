<?php

$rows = [];
$base = 1700000000;
for ($i = 0; $i < 12; $i++) {
    $o = 100 + $i;
    $h = $o + 2;
    $l = $o - 2;
    $c = $o + ($i % 2 ? -1 : 1);
    $rows[] = [$base + $i * 86400, $o, $h, $l, $c];
}

$styles = [
    'CANDLE'  => FastChart\Chart::STYLE_CANDLE,
    'BAR'     => FastChart\Chart::STYLE_BAR,
    'DIAMOND' => FastChart\Chart::STYLE_DIAMOND,
    'I_CAP'   => FastChart\Chart::STYLE_I_CAP,
];

$up   = (0x2c << 16) | (0xa0 << 8) | 0x2c;
$down = (0xd6 << 16) | (0x27 << 8) | 0x28;

foreach ($styles as $name => $style) {
    $im = imagecreatetruecolor(800, 400);
    $out = (new FastChart\StockChart)
        ->setSize(800, 400)
        ->setOhlcv($rows)
        ->setCandleStyle($style)
        ->draw($im);

    $found_up = $found_down = false;
    for ($y = 40; $y < 360; $y += 2) {
        for ($x = 70; $x < 770; $x += 2) {
            $c = imagecolorat($im, $x, $y);
            if ($c === $up)   $found_up = true;
            if ($c === $down) $found_down = true;
            if ($found_up && $found_down) break 2;
        }
    }
    echo "$name: ", ($found_up && $found_down ? "ok" : "missing"), "\n";
}

// Invalid style rejected.
try {
    (new FastChart\StockChart)->setCandleStyle(99);
    echo "bad_style: no throw\n";
} catch (\ValueError $e) {
    echo "bad_style: ValueError ok\n";
}
?>
