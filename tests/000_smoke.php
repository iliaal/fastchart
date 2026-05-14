<?php

var_dump(extension_loaded('fastchart'));
var_dump(class_exists('FastChart\\Chart'));
var_dump(class_exists('FastChart\\LineChart'));
var_dump(class_exists('FastChart\\BarChart'));
var_dump(class_exists('FastChart\\PieChart'));
var_dump(class_exists('FastChart\\ScatterChart'));
var_dump(class_exists('FastChart\\StockChart'));

// Abstract base cannot be instantiated.
try {
    new FastChart\Chart();
} catch (\Error $e) {
    echo "abstract: ", $e->getMessage(), "\n";
}

// Class constants are visible on the abstract.
var_dump(FastChart\Chart::THEME_LIGHT);
var_dump(FastChart\Chart::THEME_DARK);

// version() returns the build-time PHP_FASTCHART_VERSION.
$version = FastChart\Chart::version();
var_dump(is_string($version) && $version !== '');

// All concrete subclasses inherit Chart and chain through fluent setters.
foreach (['LineChart', 'BarChart', 'PieChart', 'ScatterChart', 'StockChart'] as $cls) {
    $fqcn = "FastChart\\$cls";
    $c = new $fqcn();
    $c->setSize(640, 480)->setTitle("hi $cls")->setTheme(FastChart\Chart::THEME_DARK);
    echo "$cls: ", get_class($c), "\n";
    var_dump($c instanceof FastChart\Chart);
}

// draw() rejects non-GdImage arguments.
try {
    (new FastChart\LineChart())->draw("not a canvas");
} catch (\TypeError $e) {
    echo "type: ", str_contains($e->getMessage(), "GdImage") ? "ok" : $e->getMessage(), "\n";
}

// draw() into a real GdImage round-trips the canvas (scaffold no-op).
$canvas = imagecreatetruecolor(100, 100);
$out = (new FastChart\LineChart())->setSeries([1, 2, 3])->draw($canvas);
var_dump($out instanceof \GdImage);
var_dump($out === $canvas);
?>
