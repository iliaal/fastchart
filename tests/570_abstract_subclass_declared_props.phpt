--TEST--
Abstract Chart/Symbol/Barcode subclasses with declared properties throw cleanly on every instantiation
--EXTENSIONS--
fastchart
--FILE--
<?php

class FCPropChart extends FastChart\Chart {
    public $a;
    public $b = [1, 2];
    public string $s;
    public ?object $o = null;
}
class FCPropSym extends FastChart\Symbol {
    public $a;
    public $b;
}
class FCPropBar extends FastChart\Barcode {
    public $a;
    public $b;
    public $c;
}
class FCSpray {
    public $a;
    public $b;
    public $c;
    public $d;
}

/* Freed objects of the same size class leave live-looking zvals behind;
 * an uninitialized property slot in the sentinel would pick them up. */
$keep = [];
$results = [];
for ($round = 0; $round < 4; $round++) {
    $objs = [];
    for ($i = 0; $i < 500; $i++) {
        $s = str_repeat(chr(65 + $i % 26), 40) . $i;
        $keep[] = $s;
        $o = new FCSpray;
        $o->a = $s; $o->b = $s; $o->c = $s; $o->d = $s;
        $objs[] = $o;
    }
    unset($o, $s);
    $objs = null;
    foreach (['FCPropChart', 'FCPropSym', 'FCPropBar'] as $cls) {
        foreach (['new', 'reflect'] as $mode) {
            for ($i = 0; $i < 50; $i++) {
                try {
                    if ($mode === 'new') {
                        $x = new $cls();
                    } else {
                        $x = (new ReflectionClass($cls))
                            ->newInstanceWithoutConstructor();
                    }
                    $results["$cls/$mode: BUG instantiated"] = true;
                } catch (\Error $e) {
                    $ok = strpos($e->getMessage(), 'is internal and cannot be'
                        . ' instantiated or subclassed') !== false;
                    $results["$cls/$mode: " . ($ok ? 'ok' : 'BAD '
                        . $e->getMessage())] = true;
                }
            }
        }
    }
}
unset($e);

$n = 0;
foreach ($keep as $s) { $n += strlen($s); }
$keep = null;
$junk = [];
for ($i = 0; $i < 5000; $i++) { $junk[] = str_repeat('Z', 48) . $i; }

foreach (array_keys($results) as $line) { echo $line, "\n"; }
echo "bytes: ", $n, "\n";
echo "concrete LineChart: ",
    (new FastChart\LineChart(100, 100))->setSeries([1, 2, 3])->renderSvg()
        ? 'ok' : 'BAD', "\n";
?>
--EXPECT--
FCPropChart/new: ok
FCPropChart/reflect: ok
FCPropSym/new: ok
FCPropSym/reflect: ok
FCPropBar/new: ok
FCPropBar/reflect: ok
bytes: 85560
concrete LineChart: ok
