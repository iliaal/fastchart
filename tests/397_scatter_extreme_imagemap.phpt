--TEST--
ScatterChart image-map coordinates match rendered markers for extreme finite X values
--EXTENSIONS--
fastchart
--FILE--
<?php

$chart = (new FastChart\ScatterChart(200, 120))
    ->setPoints([
        [-PHP_FLOAT_MAX, 0, 'href' => '/left'],
        [ PHP_FLOAT_MAX, 1, 'href' => '/right'],
    ]);
$svg = $chart->renderSvg();
$areas = $chart->getImageMapAreas();

preg_match_all('/<circle cx="(-?\d+)" cy="(-?\d+)" r="3"/', $svg, $markers,
    PREG_SET_ORDER);

foreach ($areas as $i => $area) {
    $x = $area['coords'][0];
    $y = $area['coords'][1];
    echo $area['href'], ' bounded: ',
        ($x >= 0 && $x < 200 && $y >= 0 && $y < 120 ? 'yes' : 'no'), "\n";
    echo $area['href'], ' matches marker: ',
        (isset($markers[$i]) && $x === (int)$markers[$i][1]
            && $y === (int)$markers[$i][2] ? 'yes' : 'no'), "\n";
}

?>
--EXPECT--
/left bounded: yes
/left matches marker: yes
/right bounded: yes
/right matches marker: yes
