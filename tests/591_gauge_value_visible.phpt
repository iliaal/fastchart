--TEST--
GaugeChart current-value readout stays below the hub and inside ordinary canvases
--EXTENSIONS--
fastchart
--FILE--
<?php
$cases = [
    'default' => [400, 300, 10, null, false],
    'gallery' => [480, 320, 10, null, true],
    'large' => [1200, 800, 22, null, true],
    'label override' => [480, 320, 10, 24, true],
    'large small font' => [1200, 800, 10, null, false],
    'short' => [480, 200, 10, null, false],
    'explicit plot' => [480, 320, 10, null, true],
    'high DPI' => [480, 320, 10, null, true],
];
$gallerySvg = [];
foreach ($cases as $name => [$w, $h, $base, $label, $title]) {
    foreach ([FastChart\GaugeChart::STYLE_NEEDLE, FastChart\GaugeChart::STYLE_SOLID] as $style) {
        $chart = (new FastChart\GaugeChart($w, $h))
            ->setRange(0, 100)->setValue(72)->setValueFormat('%.0f%%')
            ->setFontSize($base)->setStyle($style)
            ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
        if ($name === 'explicit plot') $chart->setPlotRect(60, 60, 420, 250);
        if ($name === 'high DPI') $chart->setDpi(144);
        if ($title) $chart->setTitle('CPU utilization');
        if ($label !== null) $chart->setLabelFont(null, $label);
        $svg = $chart->renderSvg();
        if ($name === 'gallery') $gallerySvg[$style] = $svg;
        if ($name === 'high DPI') {
            // SVG uses logical pixels; this checks invariance, not raster scaling.
            echo "high DPI style=$style SVG unchanged: ",
                $svg === $gallerySvg[$style] ? 'yes' : 'FAIL', "\n";
        }
        preg_match('/<text[^>]* y="([^"]+)"[^>]* font-size="([^"]+)"[^>]*>72%<\/text>/', $svg, $value);
        preg_match('/<circle[^>]* cy="([^"]+)"/', $svg, $hub);
        $ok = count($value) === 3 && count($hub) === 2;
        if ($ok) {
            $y = (float)$value[1];
            $size = (float)$value[2];
            // The readout stays on-canvas, including room for descenders.
            $ok = $y > (float)$hub[1] && $y - $size >= 0 && $y + $size * .25 < $h;
            if ($style === FastChart\GaugeChart::STYLE_NEEDLE) {
                preg_match_all('/<circle[^>]* cy="([^"]+)" r="([^"]+)"/', $svg, $circles, PREG_SET_ORDER);
                $needleHub = end($circles);
                $ok = $ok && $needleHub && $y - $size > (float)$needleHub[1] + (float)$needleHub[2];
            }
        }
        echo "$name style=$style: ", $ok ? 'visible' : 'FAIL', "\n";
    }
}
?>
--EXPECT--
default style=0: visible
default style=1: visible
gallery style=0: visible
gallery style=1: visible
large style=0: visible
large style=1: visible
label override style=0: visible
label override style=1: visible
large small font style=0: visible
large small font style=1: visible
short style=0: visible
short style=1: visible
explicit plot style=0: visible
explicit plot style=1: visible
high DPI style=0 SVG unchanged: yes
high DPI style=0: visible
high DPI style=1 SVG unchanged: yes
high DPI style=1: visible
