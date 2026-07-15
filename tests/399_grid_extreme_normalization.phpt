--TEST--
Surface, Heatmap, and Contour preserve geometry across extreme finite ranges
--EXTENSIONS--
fastchart
--FILE--
<?php

foreach ([FastChart\SurfaceChart::class, FastChart\Heatmap::class] as $class) {
    $svg = (new $class(240, 160))
        ->setGrid([[-PHP_FLOAT_MAX, PHP_FLOAT_MAX],
                   [-PHP_FLOAT_MAX, PHP_FLOAT_MAX]])
        ->setColorRamp(0x0000ff, 0xff0000)
        ->renderSvg();
    echo $class, ' low: ', substr_count($svg, '#0000FF') === 2 ? "yes\n" : "no\n";
    echo $class, ' high: ', substr_count($svg, '#FF0000') === 2 ? "yes\n" : "no\n";
}

$svg = (new FastChart\ContourChart(240, 160))
    ->setGrid([[-PHP_FLOAT_MAX, PHP_FLOAT_MAX / 2],
               [-PHP_FLOAT_MAX, PHP_FLOAT_MAX / 2]])
    ->setLevels([0])
    ->renderSvg();
preg_match('/<line x1="(\d+)" y1="\d+" x2="(\d+)"/', $svg, $line);
$x1 = isset($line[1]) ? (int)$line[1] : -1;
$x2 = isset($line[2]) ? (int)$line[2] : -1;
echo 'Contour crossing near two-thirds: ',
    ($x1 >= 149 && $x1 <= 151 && $x2 === $x1) ? "yes\n" : "no ($x1,$x2)\n";

?>
--EXPECT--
FastChart\SurfaceChart low: yes
FastChart\SurfaceChart high: yes
FastChart\Heatmap low: yes
FastChart\Heatmap high: yes
Contour crossing near two-thirds: yes
