--TEST--
GaugeChart: STYLE_SOLID fills a progress arc and drops the needle
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* STYLE_NEEDLE (default) draws a pointer line + hub circle. STYLE_SOLID
 * replaces both with a filled value arc in the matching zone color, so a
 * solid gauge has strictly fewer <line> elements (no needle) than the
 * needle gauge of the same data, while staying valid finite SVG. */

function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$zones = [
    ['from' => 0,  'to' => 50,  'color' => 0x2ecc71],
    ['from' => 50, 'to' => 80,  'color' => 0xf1c40f],
    ['from' => 80, 'to' => 100, 'color' => 0xe74c3c],
];

$mk = function (int $style) use ($zones) {
    return (new FastChart\GaugeChart(420, 300))
        ->setRange(0, 100)->setValue(72)->setZones($zones)
        ->setStyle($style)->renderSvg();
};

$needle = $mk(FastChart\GaugeChart::STYLE_NEEDLE);
$solid  = $mk(FastChart\GaugeChart::STYLE_SOLID);

echo "needle_valid: ", valid($needle) ? "yes" : "no", "\n";
echo "solid_valid: ", valid($solid) ? "yes" : "no", "\n";
/* The needle is a thick <line>; the solid arc has none of it. */
echo "solid_drops_needle: ",
    (substr_count($solid, '<line') < substr_count($needle, '<line') ? "yes" : "no"), "\n";
echo "solid_clean: ", (strpos($solid, '-2147483648') === false ? "yes" : "no"), "\n";

/* Out-of-range style throws a ValueError. */
try {
    (new FastChart\GaugeChart(420, 300))->setStyle(99);
    echo "bad_style: no-throw\n";
} catch (\ValueError $e) {
    echo "bad_style: threw\n";
}

echo "ok\n";
?>
--EXPECT--
needle_valid: yes
solid_valid: yes
solid_drops_needle: yes
solid_clean: yes
bad_style: threw
ok
