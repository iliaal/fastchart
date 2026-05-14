--TEST--
Chart::setJpegQuality(): range validation + default 88
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$c = new FastChart\LineChart(200, 100);
$c->setSeries([['data' => [1, 2, 3]]]);

// Valid: 1..100 inclusive. No throw.
$c->setJpegQuality(1);
$c->setJpegQuality(100);
$c->setJpegQuality(88);
echo "valid: ok\n";

// Below range.
try {
    $c->setJpegQuality(0);
    echo "no throw at 0\n";
} catch (ValueError $e) {
    echo "0: ", $e->getMessage(), "\n";
}

// Above range.
try {
    $c->setJpegQuality(101);
    echo "no throw at 101\n";
} catch (ValueError $e) {
    echo "101: ", $e->getMessage(), "\n";
}

// Symbol family mirrors the API.
$sym = new FastChart\QrCode();
$sym->setData('ok');
$sym->setJpegQuality(50);
echo "symbol setJpegQuality: ok\n";

try {
    $sym->setJpegQuality(0);
    echo "no throw on symbol\n";
} catch (ValueError $e) {
    echo "symbol 0: ", $e->getMessage(), "\n";
}

echo "OK\n";
--EXPECT--
valid: ok
0: FastChart\Chart::setJpegQuality() must be in [1, 100]
101: FastChart\Chart::setJpegQuality() must be in [1, 100]
symbol setJpegQuality: ok
symbol 0: FastChart\Symbol::setJpegQuality() must be in [1, 100]
OK
