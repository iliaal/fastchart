--TEST--
PolarChart: STYLE_ROSE with a large finite angle does not overflow the int cast
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* Regression: the ROSE branch cast (int)(360.0 - angle) % 360 without
 * normalizing, so a finite-but-large angle (e.g. 1e18) was
 * float-cast-overflow UB (traps under -fsanitize=undefined, the CI
 * build). The cast now fmod's first. The chart must render valid SVG
 * with finite coordinates for such input. */

function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$c = (new FastChart\PolarChart(400, 400))
    ->setStyle(FastChart\PolarChart::STYLE_ROSE)
    ->setSeries([['data' => [[1e18, 5], [120, 8], [240, 6]]]]);

$svg = $c->renderSvg();
echo "valid: ", valid($svg) ? "yes" : "no", "\n";
echo "finite: ", (strpos($svg, '-2147483648') === false) ? "yes" : "no", "\n";

echo "ok\n";
?>
--EXPECT--
valid: yes
finite: yes
ok
