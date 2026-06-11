--TEST--
AreaChart: secondary Y-axis maps right-axis series on an independent scale
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_8e3c2a91: AreaChart ignored per-series right_axis and never drew the
 * secondary axis. Primary-axis setYAxisRange must not clip the right scale. */

$svg = (new FastChart\AreaChart(700, 400))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSecondaryYAxis(true)
    ->setYAxisRange(0, 50)
    ->setSeries([
        ['data' => [10, 20, 30, 40], 'axis' => 'left'],
        ['data' => [100, 200, 300, 400], 'axis' => 'right'],
    ])
    ->renderSvg();

echo "right_axis_shows_400: ",
    (preg_match('/>400</', $svg) ? 'yes' : 'no'), "\n";
echo "left_axis_shows_50: ",
    (preg_match('/>50</', $svg) ? 'yes' : 'no'), "\n";

?>
--EXPECT--
right_axis_shows_400: yes
left_axis_shows_50: yes