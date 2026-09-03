--TEST--
addIconAt() rejects paths outside open_basedir at setter time
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
open_basedir=/nonexistent-fastchart-dir
--FILE--
<?php
$c = new FastChart\LineChart(64, 64);
try {
    $c->addIconAt(1, 1, __FILE__);
    echo "NO-THROW\n";
} catch (Error $e) {
    echo str_contains($e->getMessage(), 'open_basedir') ? "basedir-throw\n" : "wrong: {$e->getMessage()}\n";
}
// Sibling setter parity: setBackgroundImage throws the same way.
try {
    $c->setBackgroundImage(__FILE__);
    echo "NO-THROW\n";
} catch (Error $e) {
    echo str_contains($e->getMessage(), 'open_basedir') ? "basedir-throw\n" : "wrong: {$e->getMessage()}\n";
}
?>
--EXPECTF--
Warning: FastChart\Chart::addIconAt(): open_basedir restriction in effect.%s
basedir-throw

Warning: FastChart\Chart::setBackgroundImage(): open_basedir restriction in effect.%s
basedir-throw
