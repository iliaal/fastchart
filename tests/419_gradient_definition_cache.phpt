--TEST--
SVG gradient definitions are reused across repeated shapes
--EXTENSIONS--
fastchart
--FILE--
<?php

function gradient_ids(string $svg): array
{
    preg_match_all('/<linearGradient id="([^"]+)"/', $svg, $defs);
    preg_match_all('/fill="url\(#([^)]+)\)"/', $svg, $refs);
    return [$defs[1], $refs[1]];
}

$bar = (new FastChart\BarChart(360, 220))
    ->setSeries([8, 3, 7, 4, 6, 2])
    ->setGradientFill(
        0x112233,
        0xAABBCC,
        FastChart\Chart::GRADIENT_HORIZONTAL
    );
$bar_svg = $bar->renderSvg();
[$bar_defs, $bar_refs] = gradient_ids($bar_svg);
echo 'bar definitions: ', count($bar_defs), "\n";
echo 'bar references: ', count($bar_refs), "\n";
echo 'bar shared id: ',
    ($bar_defs === ['fcg1'] && array_unique($bar_refs) === ['fcg1'])
        ? "yes\n" : "NO\n";
echo 'bar definition preserved: ',
    (str_contains($bar_svg,
        '<linearGradient id="fcg1" x1="0%" y1="0%" x2="100%" y2="0%">')
     && str_contains($bar_svg, 'stop-color="#112233"')
     && str_contains($bar_svg, 'stop-color="#AABBCC"'))
        ? "yes\n" : "NO\n";

$area = (new FastChart\AreaChart(360, 220))
    ->setSeries([
        ['data' => [1, 3, 2, 5, 4]],
        ['data' => [2, 2, 4, 3, 5]],
        ['data' => [1, 2, 1, 2, 1]],
    ])
    ->setStacked(true)
    ->setGradientFill(0xCC3300, 0x3366CC);
$area_svg = $area->renderSvg();
[$area_defs, $area_refs] = gradient_ids($area_svg);
echo 'area definitions: ', count($area_defs), "\n";
echo 'area references: ', count($area_refs), "\n";
echo 'area shared id: ',
    ($area_defs === ['fcg1'] && array_unique($area_refs) === ['fcg1'])
        ? "yes\n" : "NO\n";

$fragment = $bar->drawSvgFragment('cached');
[$fragment_defs, $fragment_refs] = gradient_ids($fragment);
echo 'fragment namespace: ',
    ($fragment_defs === ['cachedfcg1']
     && array_unique($fragment_refs) === ['cachedfcg1'])
        ? "yes\n" : "NO\n";

?>
--EXPECT--
bar definitions: 1
bar references: 6
bar shared id: yes
bar definition preserved: yes
area definitions: 1
area references: 3
area shared id: yes
fragment namespace: yes
