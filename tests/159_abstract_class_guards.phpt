--TEST--
Abstract Chart and Symbol classes reject direct + userland-subclass instantiation
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Named class declarations don't instantiate; the `new ($cls::class)`
 * lines below are the actual instantiation points. */
class FCSubA extends FastChart\Chart {}
class FCSubB extends FastChart\Chart {
    public function __construct() { echo "BUG: userland ctor ran\n"; }
}
class FCSubS extends FastChart\Symbol {
    public function __construct() { echo "BUG: sym ctor ran\n"; }
}

/* Direct instantiation of abstract Chart: caught by the engine's
 * own ZEND_ACC_EXPLICIT_ABSTRACT_CLASS check. */
try {
    new FastChart\Chart();
    echo "BUG: direct Chart instantiation allowed\n";
} catch (\Error $e) {
    echo "direct Chart: ", strpos($e->getMessage(), 'abstract') !== false ? 'ok' : 'BAD', "\n";
}

/* Userland subclass without a constructor: blocked by our sentinel
 * create_object. */
try {
    new FCSubA(100, 100);
    echo "BUG: userland subclass instantiation succeeded\n";
} catch (\Error $e) {
    echo "userland subclass: ", strpos($e->getMessage(), 'internal') !== false ? 'ok' : 'BAD', "\n";
}

/* Userland subclass with its own __construct — the previous bug
 * class. The sentinel must prevent the inherited userland
 * constructor from running on a vanilla zend_object that lacks
 * FASTCHART_BASE_FIELDS. */
try {
    new FCSubB();
    echo "BUG: subclass-with-ctor allowed\n";
} catch (\Error $e) {
    echo "subclass with own ctor: ", strpos($e->getMessage(), 'internal') !== false ? 'ok' : 'BAD', "\n";
}

/* Same coverage on the Symbol family. */
try {
    new FastChart\Symbol();
    echo "BUG: direct Symbol\n";
} catch (\Error $e) {
    echo "direct Symbol: ", strpos($e->getMessage(), 'abstract') !== false ? 'ok' : 'BAD', "\n";
}

try {
    new FastChart\Barcode();
    echo "BUG: direct Barcode\n";
} catch (\Error $e) {
    echo "direct Barcode: ", strpos($e->getMessage(), 'abstract') !== false ? 'ok' : 'BAD', "\n";
}

try {
    new FCSubS();
    echo "BUG: userland Symbol subclass\n";
} catch (\Error $e) {
    echo "userland Symbol+ctor: ", strpos($e->getMessage(), 'internal') !== false ? 'ok' : 'BAD', "\n";
}

/* Concrete classes still work. */
echo "concrete LineChart: ", (new FastChart\LineChart(100, 100))->setSeries([1,2,3])->renderSvg() ? 'ok' : 'BAD', "\n";
echo "concrete Code128: ", strlen((new FastChart\Code128())->setData('HELLO')->renderSvg()) > 100 ? 'ok' : 'BAD', "\n";
?>
--EXPECT--
direct Chart: ok
userland subclass: ok
subclass with own ctor: ok
direct Symbol: ok
direct Barcode: ok
userland Symbol+ctor: ok
concrete LineChart: ok
concrete Code128: ok
