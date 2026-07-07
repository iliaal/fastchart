--TEST--
LineChart::addHorizontalBand() spans the plot width across the mapped value range
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: addHorizontalBand() had zero references. A band from
 * value low..high draws a translucent rect spanning the plot width,
 * its vertical extent mapped through the y-axis. With a forced axis
 * the pixel edges are exact: top = ypix(high), bottom = ypix(low). */

function plot_rect(string $svg): array {
    preg_match_all('/<rect x="([0-9.-]+)" y="([0-9.-]+)" width="([0-9.-]+)" height="([0-9.-]+)" fill="#FFFFFF"/',
        $svg, $r, PREG_SET_ORDER);
    $p = $r[1];
    return [(float)$p[1], (float)$p[2], (float)$p[3], (float)$p[4]]; /* x,y,w,h */
}
function ypix(float $v, float $min, float $max, float $y0, float $h_rect): int {
    $h = $h_rect - 1; $y1 = $y0 + $h;
    $f = ($v - $min) / ($max - $min);
    if ($f < 0) $f = 0; if ($f > 1) $f = 1;
    return (int)($y1 - (int)($f * $h + 0.5));
}

$MIN = 0.0; $MAX = 40.0; $LOW = 10.0; $HIGH = 30.0;
$svg = (new FastChart\LineChart(400, 300))
    ->setSeries([10, 20, 15, 30, 25])
    ->setYAxisRange($MIN, $MAX)
    ->addHorizontalBand($LOW, $HIGH, 0x00AAFF)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->renderSvg();

[$px, $py, $pw, $ph] = plot_rect($svg);

/* The band rect carries the colour as translucent rgba. */
if (!preg_match('/<rect x="([0-9.-]+)" y="([0-9.-]+)" width="([0-9.-]+)" height="([0-9.-]+)" fill="rgba\(0,170,255,[0-9.]+\)"\/>/',
        $svg, $b)) {
    echo "band rect not found\n";
    return;
}
$bx = (float)$b[1]; $by = (float)$b[2]; $bw = (float)$b[3]; $bh = (float)$b[4];

/* Spans the plot width (inset by the 1px plot border on each side). */
echo "spans_plot_width: ", ($bx == $px + 1 && $bw == $pw - 2 ? 'yes' : 'no'), "\n";
/* Vertical extent maps high -> top, low -> bottom of the band. */
echo "top_at_high: ", ((int)$by === ypix($HIGH, $MIN, $MAX, $py, $ph) ? 'yes' : 'no'), "\n";
echo "bottom_at_low: ", ((int)($by + $bh - 1) === ypix($LOW, $MIN, $MAX, $py, $ph) ? 'yes' : 'no'), "\n";
?>
--EXPECT--
spans_plot_width: yes
top_at_high: yes
bottom_at_low: yes
