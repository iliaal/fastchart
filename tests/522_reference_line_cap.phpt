--TEST--
Reference lines share a 128-line cap and a 64 KiB label budget without changing prior state
--EXTENSIONS--
fastchart
--FILE--
<?php

function attempt(callable $fn): string {
    try {
        $fn();
        return "accepted";
    } catch (ValueError $e) {
        return $e->getMessage();
    }
}

$c = (new FastChart\LineChart(400, 300))->setSeries([1, 2, 3]);
for ($i = 0; $i < 64; $i++) {
    $c->addHorizontalLine(2.0);
    $c->addVerticalLine(1.0);
}
$before = $c->renderSvg();
echo "129th horizontal: ", attempt(fn() => $c->addHorizontalLine(2.0)), "\n";
echo "129th vertical: ", attempt(fn() => $c->addVerticalLine(1.0)), "\n";
echo "state: ", $c->renderSvg() === $before ? "preserved" : "CHANGED", "\n";

$full = str_repeat('W', 8192);
$c = (new FastChart\BarChart(400, 300))->setSeries([1, 2, 3]);
for ($i = 0; $i < 4; $i++) {
    $c->addHorizontalLine(2.0, $full);
    $c->addVerticalLine(1.0, $full);
}
echo "exact budget: accepted\n";
$c->addHorizontalLine(1.5, '');
echo "empty label at budget: accepted\n";
echo "one more byte: ", attempt(fn() => $c->addVerticalLine(0.5, 'x')), "\n";
echo "unlabeled after budget: ", attempt(fn() => $c->addHorizontalLine(2.5)), "\n";
?>
--EXPECT--
129th horizontal: FastChart\Chart::addHorizontalLine() accepts at most 128 reference lines
129th vertical: FastChart\Chart::addVerticalLine() accepts at most 128 reference lines
state: preserved
exact budget: accepted
empty label at budget: accepted
one more byte: FastChart\Chart::addVerticalLine() aggregate text exceeds the 65536-byte limit
unlabeled after budget: accepted
