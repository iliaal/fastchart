--TEST--
clone produces an isolated chart: setters on either side don't disturb the other
--EXTENSIONS--
fastchart
--FILE--
<?php

// Common case: shape the original, clone it, mutate the clone, render
// both. The two byte streams must differ (the clone applied a new
// title) and neither must crash or corrupt the other's state.
$orig = (new FastChart\LineChart(400, 300))
    ->setSeries([1, 2, 3, 4, 5])
    ->setTitle('original');

$copy = clone $orig;
$copy->setTitle('the clone');

$orig_bytes = $orig->renderPng();
$copy_bytes = $copy->renderPng();
echo "orig_renders: ", strlen($orig_bytes) > 1024 ? "ok" : "bad", "\n";
echo "copy_renders: ", strlen($copy_bytes) > 1024 ? "ok" : "bad", "\n";
echo "differ: ",       $orig_bytes !== $copy_bytes ? "yes" : "no",  "\n";

// Mutating the clone's data array does not surface in the original.
$o2 = (new FastChart\LineChart(400, 300))->setSeries([10, 20, 30]);
$c2 = clone $o2;
$c2->setSeries([99, 99, 99]);
$o2_b = $o2->renderPng();
$c2_b = $c2->renderPng();
echo "data_isolated: ", $o2_b !== $c2_b ? "yes" : "no", "\n";

// Title-only divergence: identical data, only the title differs.
$o3 = (new FastChart\LineChart(300, 200))
    ->setSeries([1, 2, 3])
    ->setTitle('hi');
$c3 = clone $o3;
$c3->setTitle('there');
echo "title_isolated: ", $o3->renderPng() !== $c3->renderPng() ? "yes" : "no", "\n";

// Free the original first, then render the clone — the clone must
// still own its strings.
$o4 = (new FastChart\BarChart(300, 200))
    ->setSeries([5, 10, 15])
    ->setTitle('parent');
$c4 = clone $o4;
unset($o4);
$bytes = $c4->renderPng();
echo "clone_outlives_original: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
orig_renders: ok
copy_renders: ok
differ: yes
data_isolated: yes
title_isolated: yes
clone_outlives_original: ok
