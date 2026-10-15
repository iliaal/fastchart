--TEST--
Shared numeric and time axes preserve complete tick labels
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
$c = (new FastChart\LineChart(600,400))->setSeries([1,2])->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
if (!str_contains($c->renderSvg(), '<text ')) echo 'skip no usable default font';
?>
--FILE--
<?php
use FastChart\Chart;
function texts(string $svg): array {
    preg_match_all('/<text\b[^>]*>([^<]*)<\/text>/', $svg, $m);
    return array_map(fn($s) => html_entity_decode($s, ENT_QUOTES | ENT_XML1, 'UTF-8'), $m[1]);
}
$rows = [[1700000000,50,60,40,55],[1700172800,55,65,45,60]];
$tasks = [['name'=>'a','start'=>1700000000,'end'=>1700172800]];
$cases = [
    'Line Y' => [(new FastChart\LineChart(600,400))->setSeries([50,-25,0]), 'Y'],
    'Area Y' => [(new FastChart\AreaChart(600,400))->setSeries([50,-25,0]), 'Y'],
    'Bar Y' => [(new FastChart\BarChart(600,400))->setSeries([50,-25,0]), 'Y'],
    'Bubble Y' => [(new FastChart\BubbleChart(600,400))->setPoints([[1,50,5],[2,-25,5]]), 'Y'],
    'BoxPlot Y' => [(new FastChart\BoxPlot(600,400))->setBoxes([[0,10,20,30,50]]), 'Y'],
    'Waterfall Y' => [(new FastChart\Waterfall(600,400))->setBars([['value'=>50],['value'=>-25]]), 'Y'],
    'Stock Y' => [(new FastChart\StockChart(600,400))->setOhlcv($rows), 'Y'],
    'Scatter Y' => [(new FastChart\ScatterChart(600,400))->setPoints([[0,50],[50,-25]]), 'Y'],
    'Scatter X' => [(new FastChart\ScatterChart(600,400))->setPoints([[0,50],[50,-25]]), 'X'],
    'Bar X' => [(new FastChart\BarChart(600,400))->setSeries([50,-25,0])->setOrientation(FastChart\BarChart::BAR_HORIZONTAL), 'X'],
];
foreach (['LineChart','AreaChart'] as $class) {
    $class = 'FastChart\\'.$class;
    $cases["$class secondary"] = [(new $class(600,400))->setSeries([
        ['data'=>[50,-25]], ['data'=>[100,0],'axis'=>'right']
    ])->setSecondaryYAxis(true), 'Y'];
}
foreach ([0,45,90] as $angle) {
    foreach (['Stock','Gantt'] as $kind) {
        $c = $kind === 'Stock' ? (new FastChart\StockChart(600,400))->setOhlcv($rows)
            : (new FastChart\GanttChart(600,400))->setTasks($tasks);
        $c->setXAxisLabelAngle($angle);
        $cases["$kind automatic $angle"] = [clone $c, 'X'];
        $cases["$kind calendar $angle"] = [$c->setDateAxisStride(Chart::DATE_DAY,1), 'X'];
    }
}
$formats = ['v%30.0f','v%31.0f','v%32.0f','v%62.0f','v%63.0f','v%64.0f','v%100.2f','v%.300f','v<%.2f>&%%'];
foreach ($cases as $name => [$c,$axis]) {
    $c->setSvgTextMode(Chart::SVG_TEXT_NATIVE);
    $setter = 'set'.$axis.'AxisLabelFormat';
    $default = $c->renderSvg();
    $reference = $c->$setter('v%.0f')->renderSvg();
    $values = array_map(fn($s)=>(float)substr($s,1), array_values(array_filter(texts($reference), fn($s)=>str_starts_with($s,'v'))));
    $ok = count($values) > 0;
    foreach ($formats as $fmt) {
        $svg = $c->$setter($fmt)->renderSvg();
        $expected = array_map(fn($v)=>$fmt === 'v%.300f' ? 'v'.sprintf('%.0f',$v).'.'.str_repeat('0',300) : sprintf($fmt,$v), $values);
        $actual = array_values(array_filter(texts($svg), fn($s)=>str_starts_with($s,'v')));
        if ($actual !== $expected) { echo "$name $fmt: incomplete\n"; $ok = false; }
        $ok = $ok && $svg === $c->renderSvg() && $svg === (clone $c)->renderSvg();
    }
    $ok = $ok && $default === $c->$setter('')->renderSvg();
    foreach (['hidden','points','thumbnail'] as $mode) {
        $off = clone $c;
        if ($mode === 'hidden') $off->{'set'.$axis.'AxisVisible'}(false);
        elseif ($mode === 'points') $off->setTickMode(Chart::TICK_POINTS);
        else $off->setThumbnailMode(true);
        $plain = $off->renderSvg();
        $long = $off->$setter('v%.300f')->renderSvg();
        $ok = $ok && $plain === $long;
    }
    echo "$name: ", $ok ? 'complete and stable' : 'FAIL', "\n";
}
// Huge numeric defaults switch to compact exponent labels of the same ticks.
$huge = [
    'primary' => (new FastChart\LineChart(600,400))->setSeries([1e100])->setYAxisRange(0,1e100)->setXAxisVisible(false),
    'secondary' => (new FastChart\LineChart(600,400))->setSeries([['data'=>[1e100]],['data'=>[1e100],'axis'=>'right']])->setSecondaryYAxis(true)->setXAxisVisible(false),
    'numeric X' => (new FastChart\BarChart(600,400))->setSeries([1e100])->setYAxisRange(0,1e100)->setOrientation(FastChart\BarChart::BAR_HORIZONTAL)->setYAxisVisible(false),
];
foreach ($huge as $name=>$c) {
    $c->setSvgTextMode(Chart::SVG_TEXT_NATIVE);
    $setter = $name === 'numeric X' ? 'setXAxisLabelFormat' : 'setYAxisLabelFormat';
    $default = $c->renderSvg();
    $scientific = texts($c->$setter('%.17g')->renderSvg());
    $actual = texts($default);
    $ok = count($actual) === count($scientific) && count($actual) > 1;
    foreach ($scientific as $i => $ref) {
        $label = $actual[$i] ?? '';
        if (!is_numeric($ref)) { $ok = $ok && $label === $ref; continue; }
        $ok = $ok && preg_match('/^(0|-?[1-9](\.\d+)?e[+-]\d{1,3})$/', $label)
            && abs((float)$label - (float)$ref) <= 1e-12 * abs((float)$ref);
    }
    echo "huge $name: ", $ok ? 'complete' : 'FAIL', "\n";
}

?>
--EXPECT--
Line Y: complete and stable
Area Y: complete and stable
Bar Y: complete and stable
Bubble Y: complete and stable
BoxPlot Y: complete and stable
Waterfall Y: complete and stable
Stock Y: complete and stable
Scatter Y: complete and stable
Scatter X: complete and stable
Bar X: complete and stable
FastChart\LineChart secondary: complete and stable
FastChart\AreaChart secondary: complete and stable
Stock automatic 0: complete and stable
Stock calendar 0: complete and stable
Gantt automatic 0: complete and stable
Gantt calendar 0: complete and stable
Stock automatic 45: complete and stable
Stock calendar 45: complete and stable
Gantt automatic 45: complete and stable
Gantt calendar 45: complete and stable
Stock automatic 90: complete and stable
Stock calendar 90: complete and stable
Gantt automatic 90: complete and stable
Gantt calendar 90: complete and stable
huge primary: complete
huge secondary: complete
huge numeric X: complete
