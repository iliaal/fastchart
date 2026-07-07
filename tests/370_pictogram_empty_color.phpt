--TEST--
Pictogram::setEmptyColor() paints exactly the unfilled icons
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Coverage gap: setEmptyColor() had zero references. value/total =
 * 30/100 over 10 icons fills 3 and leaves 7 empty, so the empty
 * colour must appear on exactly 7 icons and the fill colour on 3.
 * Distinct hex keeps the split unambiguous; positive inputs only. */

$svg = (new FastChart\Pictogram(400, 300))
    ->setValue(30)->setTotal(100)->setIconCount(10)
    ->setFillColor(0x111FFF)
    ->setEmptyColor(0xCC33AA)
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->renderSvg();

echo "filled: ", substr_count($svg, 'fill="#111FFF"'), "\n";
echo "empty: ",  substr_count($svg, 'fill="#CC33AA"'), "\n";
echo "total_icons: ", substr_count($svg, 'fill="#111FFF"') + substr_count($svg, 'fill="#CC33AA"'), "\n";
?>
--EXPECT--
filled: 3
empty: 7
total_icons: 10
