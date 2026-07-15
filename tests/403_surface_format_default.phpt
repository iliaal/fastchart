--TEST--
SurfaceChart omitted cell-value format resets to the declared percent-g default
--EXTENSIONS--
fastchart
--FILE--
<?php

$svg = (new FastChart\SurfaceChart(240, 160))
    ->setGrid([[1, 2], [3, 4]])
    ->setShowCellValues(true, '%.2f')
    ->setShowCellValues(true)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->renderSvg();

echo 'uses default: ',
    (str_contains($svg, '>1</text>') && !str_contains($svg, '>1.00</text>'))
        ? "yes\n" : "no\n";

?>
--EXPECT--
uses default: yes
