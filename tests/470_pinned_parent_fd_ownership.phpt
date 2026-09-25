--TEST--
Pinned parent descriptor ownership remains bounded across repeated setters
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (!is_dir('/proc/self/fd')) die("skip /proc/self/fd unavailable\n");
?>
--FILE--
<?php

$chart = (new FastChart\LineChart(120, 80))
    ->setFontPath(__FILE__)
    ->setBackgroundImage(__FILE__);
$before = count(scandir('/proc/self/fd'));
for ($i = 0; $i < 200; $i++) {
    $chart->setFontPath(__FILE__)->setBackgroundImage(__FILE__);
}
$after = count(scandir('/proc/self/fd'));
echo ($after - $before <= 1 ? "fd-ownership-ok\n" : "fd-ownership-leaked\n");

?>
--EXPECT--
fd-ownership-ok
