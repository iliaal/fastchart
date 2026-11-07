--TEST--
WordCloud: canvas backgrounds honor image, transparency and plot-rectangle settings
--EXTENSIONS--
fastchart
--FILE--
<?php
function background(string $svg): string {
    $canvas = preg_match('/<rect x="0" y="0" width="300" height="200"/', $svg);
    return ($canvas ? 'fill' : 'clear') . ', images=' . substr_count($svg, '<image');
}
$chart = (new FastChart\WordCloud(300, 200))
    ->setWords([['text' => 'alpha', 'weight' => 10], ['text' => 'beta', 'weight' => 5]]);
echo 'default: ', background($chart->renderSvg()), "\n";
$chart->setBackgroundImage(__DIR__ . '/__icon.png');
echo 'image: ', background($chart->renderSvg()), "\n";
echo 'image fragment: ', background($chart->drawSvgFragment()), "\n";
$first = $chart->renderSvg();
echo 'repeat/clone: ', $first === $chart->renderSvg() && $first === (clone $chart)->renderSvg()
    ? "same\n" : "different\n";
$chart->setPlotRect(20, 20, 200, 150);
echo 'plot rect: ', background($chart->renderSvg()), "\n";
echo 'plot fragment: ', background($chart->drawSvgFragment()), "\n";
$chart->setTransparentBackground(true);
echo 'plot transparent: ', background($chart->renderSvg()), "\n";
$chart->setPlotRect(0, 0, -1, -1);
echo 'transparent: ', background($chart->renderSvg()), "\n";
$chart->setTransparentBackground(false);
echo 'restored: ', background($chart->renderSvg()), "\n";
$chart->setBackgroundImage('');
echo 'cleared image: ', background($chart->renderSvg()), "\n";
$chart->setPlotRect(20, 20, 200, 150);
echo 'plain plot fragment: ', background($chart->drawSvgFragment()), "\n";
?>
--EXPECT--
default: fill, images=0
image: fill, images=1
image fragment: fill, images=1
repeat/clone: same
plot rect: clear, images=0
plot fragment: clear, images=0
plot transparent: clear, images=0
transparent: clear, images=0
restored: fill, images=1
cleared image: fill, images=0
plain plot fragment: clear, images=0
