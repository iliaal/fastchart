--TEST--
LineChart::setFontSize() scales emitted text proportionally
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: setFontSize() had zero references. In native text
 * mode the requested size is emitted as the font-size attribute
 * (scaled to the 96-DPI baseline). Doubling the input must double the
 * emitted size, and every text element in one render shares one size. */

function font_size(float $fs): float {
    $svg = (new FastChart\LineChart(400, 300))
        ->setSeries([10, 20, 15])
        ->setFontSize($fs)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->renderSvg();
    preg_match_all('/font-size="([0-9.]+)"/', $svg, $m);
    $sizes = array_unique($m[1]);
    if (count($sizes) !== 1) {
        echo "MULTIPLE SIZES\n";
    }
    return (float)reset($sizes);
}

$small = font_size(10.0);
$large = font_size(20.0);

echo "differ: ", ($small !== $large ? 'yes' : 'no'), "\n";
echo "small_lt_large: ", ($small < $large ? 'yes' : 'no'), "\n";
/* 20 / 10 == 2, so the emitted sizes scale by ~2 (allow rounding). */
$ratio = $large / $small;
echo "ratio_is_2x: ", (abs($ratio - 2.0) < 0.05 ? 'yes' : "no ($ratio)"), "\n";
?>
--EXPECT--
differ: yes
small_lt_large: yes
ratio_is_2x: yes
