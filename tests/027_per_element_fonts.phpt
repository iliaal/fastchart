--TEST--
setTitleFont / setAxisFont / setLabelFont (path + size) overrides
--EXTENSIONS--
fastchart
--FILE--
<?php

// Per-element font sizes apply.
$bytes = (new FastChart\LineChart(400, 300))
    ->setTitle('Big title')
    ->setTitleFont(null, 28.0)
    ->setSeries([1, 5, 3, 8])
    ->renderPng();
echo "title_font_size: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Pass null path explicitly to keep global font, change only size.
$bytes = (new FastChart\LineChart(400, 300))
    ->setAxisFont(null, 6.0)
    ->setSeries([1, 5, 3, 8])
    ->renderPng();
echo "axis_size_only: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Label font size separately.
$bytes = (new FastChart\LineChart(400, 300))
    ->setLabelFont(null, 14.0)
    ->setCategoryLabels(['Q1', 'Q2', 'Q3', 'Q4'])
    ->setSeries([10, 20, 30, 25])
    ->renderPng();
echo "label_size: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

// Out-of-range size rejected.
try {
    (new FastChart\LineChart)->setTitleFont(null, 0.5);
    echo "small_size: no throw\n";
} catch (\ValueError $e) {
    echo "small_size: ValueError ok\n";
}
try {
    (new FastChart\LineChart)->setLabelFont(null, 999.0);
    echo "huge_size: no throw\n";
} catch (\ValueError $e) {
    echo "huge_size: ValueError ok\n";
}

// Embedded NUL in font path rejected.
try {
    (new FastChart\LineChart)->setTitleFont("/path\0/etc/passwd");
    echo "nul_in_path: no throw\n";
} catch (\ValueError $e) {
    echo "nul_in_path: ValueError ok\n";
}

// Empty string path keeps the size update without changing path.
$out = (new FastChart\LineChart)->setTitleFont("", 16.0);
var_dump($out instanceof FastChart\LineChart);
?>
--EXPECT--
title_font_size: ok
axis_size_only: ok
label_size: ok
small_size: ValueError ok
huge_size: ValueError ok
nul_in_path: ValueError ok
bool(true)
