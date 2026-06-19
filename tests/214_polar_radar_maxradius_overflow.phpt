--TEST--
PolarChart/RadarChart: a max-radius set below the data must not overflow the int cast
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* setMaxRadius() / setMaxValue() can be set far below the data radii.
 * Without a clamp, the scaled pixel radius radius*r/rmax exceeds INT_MAX
 * and the (int) cast is float-cast-overflow UB (trapped by the UBSan CI
 * build). Every render below feeds a tiny max with large data and must
 * still emit clean, finite, well-formed coordinates. */
function clean(string $svg): bool {
    foreach (['-2147483648', 'NaN', 'nan', 'inf', '="-2', '="-1.#'] as $bad) {
        if (strpos($svg, $bad) !== false) return false;
    }
    return simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

/* Polar line/area path. */
$p = (new FastChart\PolarChart(400, 400))
    ->setSeries([['data' => [[0, 1e9], [90, 5e8], [180, 1e9], [270, 7e8]]]])
    ->setMaxRadius(0.0001)
    ->renderSvg();
echo "polar_line_clean: ", clean($p) ? "yes" : "no", "\n";

/* Polar ROSE wedge path (distinct cast site). */
$rose = (new FastChart\PolarChart(400, 400))
    ->setStyle(FastChart\PolarChart::STYLE_ROSE)
    ->setSeries([['data' => [[0, 1e9], [120, 8e8], [240, 1e9]]]])
    ->setMaxRadius(0.0001)
    ->renderSvg();
echo "polar_rose_clean: ", clean($rose) ? "yes" : "no", "\n";

/* Polar vector overlay path (distinct cast site). */
$vec = (new FastChart\PolarChart(400, 400))
    ->setSeries([['data' => [[0, 1], [90, 1]]]])
    ->addVectors([['angle' => 0, 'radius' => 1e9, 'angle_to' => 90, 'radius_to' => 8e8]])
    ->setMaxRadius(0.0001)
    ->renderSvg();
echo "polar_vector_clean: ", clean($vec) ? "yes" : "no", "\n";

/* Radar shares the pattern via setMaxValue(). */
$r = (new FastChart\RadarChart(400, 400))
    ->setSeries([['data' => [1e9, 5e8, 1e9, 7e8, 8e8]]])
    ->setCategoryLabels(['a', 'b', 'c', 'd', 'e'])
    ->setMaxValue(0.0001)
    ->renderSvg();
echo "radar_clean: ", clean($r) ? "yes" : "no", "\n";

echo "ok\n";
?>
--EXPECT--
polar_line_clean: yes
polar_rose_clean: yes
polar_vector_clean: yes
radar_clean: yes
ok
