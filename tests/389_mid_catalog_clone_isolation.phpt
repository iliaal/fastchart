--TEST--
Mid-catalog charts and symbols clone heap state independently
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

function valid_svg(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$rows = [];
$ts = strtotime('2026-01-01');
for ($i = 0; $i < 20; $i++) {
    $open = 100 + $i;
    $close = $open + (($i % 3) - 1) * 1.5;
    $rows[] = [$ts + $i * 86400, $open, max($open, $close) + 2, min($open, $close) - 2, $close, 1000 + $i];
}

$builders = [
    'StockChart' => fn() => (new FastChart\StockChart(520, 320))
        ->setOhlcv($rows)
        ->addMovingAverage(5)
        ->addRSI(5),
    'Treemap' => fn() => (new FastChart\Treemap(360, 240))
        ->setItems([
            ['label' => 'A', 'value' => 5],
            ['label' => 'B', 'value' => 3],
            ['label' => 'C', 'value' => 2],
        ]),
    'SankeyChart' => fn() => (new FastChart\SankeyChart(420, 240))
        ->setNodes([
            ['label' => 'A'], ['label' => 'B'], ['label' => 'C'],
        ])
        ->setLinks([
            ['from' => 0, 'to' => 1, 'value' => 4],
            ['from' => 1, 'to' => 2, 'value' => 2],
        ]),
    'GaugeChart' => fn() => (new FastChart\GaugeChart(320, 220))
        ->setRange(0, 100)
        ->setValue(64)
        ->setZones([
            ['from' => 0, 'to' => 60, 'color' => 0x33AA66],
            ['from' => 60, 'to' => 100, 'color' => 0xDD9944],
        ]),
    'Heatmap' => fn() => (new FastChart\Heatmap(360, 240))
        ->setGrid([[1, 2, 3], [4, 5, 6]]),
    'Pictogram' => fn() => (new FastChart\Pictogram(300, 220))
        ->setValue(7)
        ->setTotal(10)
        ->setIconCount(10)
        ->setColumns(5)
        ->setShape(FastChart\Pictogram::SHAPE_CIRCLE)
        ->setFillColor(0x2266AA)
        ->setEmptyColor(0xCCCCCC),
    'Code128' => fn() => (new FastChart\Code128())
        ->setData('FASTCHART-123')
        ->setSize(320, 100)
        ->setShowText(true)
        ->setSvgTextMode(FastChart\Symbol::SVG_TEXT_NATIVE),
    'QrCode' => fn() => (new FastChart\QrCode())
        ->setData('https://example.com/fastchart')
        ->setSize(260, 260)
        ->setEcc(FastChart\QrCode::ECC_Q),
];

foreach ($builders as $name => $build) {
    $ref = $build()->renderSvg();
    $orig = $build();
    $copy = clone $orig;
    unset($orig);
    $out = $copy->renderSvg();
    echo "$name: ", (valid_svg($out) && $out === $ref ? "ok" : "BAD"), "\n";
}

?>
--EXPECT--
StockChart: ok
Treemap: ok
SankeyChart: ok
GaugeChart: ok
Heatmap: ok
Pictogram: ok
Code128: ok
QrCode: ok
