--TEST--
renderGif / renderAvif raise Error in v1.0 (formats dropped)
--EXTENSIONS--
fastchart
--FILE--
<?php
$c = (new FastChart\LineChart(200, 100))->setSeries([1, 2, 3]);

try {
    $c->renderGif();
    echo "gif: no throw\n";
} catch (\Error $e) {
    echo "gif: dropped ok\n";
}

try {
    $c->renderAvif();
    echo "avif: no throw\n";
} catch (\Error $e) {
    echo "avif: dropped ok\n";
}

echo "OK\n";
?>
--EXPECT--
gif: dropped ok
avif: dropped ok
OK
