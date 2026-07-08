--TEST--
Review follow-ups: open_basedir, caps, symbol fragment prefixes, long labels, smooth polyline
--EXTENSIONS--
fastchart
simplexml
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') echo "skip: no system font present\n";
?>
--FILE--
<?php
require __DIR__ . '/_font_candidates.inc';

function expect_value_error(string $label, callable $fn): void {
    try {
        $fn();
        echo "$label: NO THROW\n";
    } catch (ValueError $e) {
        echo "$label: throws\n";
    }
}

/* setFloating(false) rejects the ambiguous state with a caller-facing
 * message that describes disabling floating mode, not replacing series. */
try {
    (new FastChart\BarChart(300, 200))
        ->setFloating(true)
        ->setSeries([[1, 3], [2, 5]])
        ->setFloating(false);
    echo "floating_message: NO THROW\n";
} catch (ValueError $e) {
    echo "floating_message: ",
        (str_contains($e->getMessage(), 'cannot be used after setSeries') ? "ok" : "BAD"),
        "\n";
}

/* Symbol fragments accept the same optional idPrefix shape as Chart. */
$code = (new FastChart\Code128())->setData('ABC123');
echo "symbol_prefix_accepts: ",
    (str_starts_with($code->drawSvgFragment('sym1'), '<g ') ? "yes" : "NO"),
    "\n";
expect_value_error('symbol_prefix_digit', fn() => $code->drawSvgFragment('1bad'));
expect_value_error('symbol_prefix_long', fn() => $code->drawSvgFragment(str_repeat('a', 17)));
expect_value_error('symbol_prefix_char', fn() => $code->drawSvgFragment('bad.dot'));

/* Representative public caps throw instead of silently truncating. */
expect_value_error('category_cap',
    fn() => (new FastChart\LineChart())->setCategoryLabels(array_fill(0, 4097, 'x')));
expect_value_error('pie_slice_cap',
    fn() => (new FastChart\PieChart())->setSlices(array_fill(0, 33, ['value' => 1])));
expect_value_error('funnel_stage_cap',
    fn() => (new FastChart\Funnel())->setStages(array_fill(0, 33, ['value' => 1])));
expect_value_error('meter_zone_cap',
    fn() => (new FastChart\LinearMeter())->setZones(array_fill(0, 9, ['from' => 0, 'to' => 1])));
expect_value_error('bullet_band_cap',
    fn() => (new FastChart\BulletChart())->setBands(array_fill(0, 9, ['from' => 0, 'to' => 1])));
expect_value_error('word_cap',
    fn() => (new FastChart\WordCloud())->setWords(array_fill(0, 257, ['text' => 'x', 'weight' => 1])));
expect_value_error('violin_value_cap',
    fn() => (new FastChart\ViolinPlot())->setGroups([['values' => array_fill(0, 8193, 1.0)]]));
expect_value_error('hierarchy_node_cap',
    fn() => (new FastChart\CirclePacking())->setHierarchy([
        'children' => array_fill(0, 2048, ['value' => 1]),
    ]));

/* Pie / Gauge / Bullet now use the same right-sized formatter as
 * LinearMeter; accepted high-precision formats must not truncate. */
$pie = (new FastChart\PieChart(320, 240))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSlices([['label' => 'all', 'value' => 1]])
    ->setSliceLabelFormat('%.300f')
    ->renderSvg();
echo "pie_long_label: ", (preg_match('/100\.[0-9]{250,}/', $pie) ? "yes" : "NO"), "\n";

$gauge = (new FastChart\GaugeChart(360, 240))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setRange(0, 100)
    ->setValue(50)
    ->setValueFormat('%.300f')
    ->renderSvg();
echo "gauge_long_label: ", (preg_match('/50\.[0-9]{250,}/', $gauge) ? "yes" : "NO"), "\n";

$bullet = (new FastChart\BulletChart(420, 180))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setRange(0, 100)
    ->setValue(50)
    ->setTarget(75)
    ->setValueFormat('%.300f')
    ->renderSvg();
echo "bullet_long_label: ", (preg_match('/75\.[0-9]{250,}/', $bullet) ? "yes" : "NO"), "\n";

/* Smooth lines are emitted as one densified polyline per run, avoiding
 * the old many-<line> segment output. */
$smooth = (new FastChart\LineChart(360, 220))
    ->setMarkerStyle(FastChart\Chart::MARKER_NONE)
    ->setXAxisVisible(false)
    ->setYAxisVisible(false)
    ->setLineInterpolation(FastChart\Chart::INTERP_SMOOTH)
    ->setSeries([1, 5, 1, 5, 1])
    ->renderSvg();
echo "smooth_polyline: ", (substr_count($smooth, '<polyline') >= 1 ? "yes" : "NO"), "\n";
echo "smooth_valid_svg: ",
    (simplexml_load_string($smooth, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false ? "yes" : "NO"),
    "\n";

/* setFontPath() now matches peer path APIs: open_basedir denial is a
 * hard error, not a fluent no-op. Keep this last because runtime
 * open_basedir narrowing is effectively one-way for this process. */
$font = fc_pick_font();
ini_set('open_basedir', '/tmp');
try {
    @(new FastChart\LineChart(200, 120))->setFontPath($font);
    echo "font_open_basedir: NO THROW\n";
} catch (Error $e) {
    echo "font_open_basedir: throws\n";
}

?>
--EXPECT--
floating_message: ok
symbol_prefix_accepts: yes
symbol_prefix_digit: throws
symbol_prefix_long: throws
symbol_prefix_char: throws
category_cap: throws
pie_slice_cap: throws
funnel_stage_cap: throws
meter_zone_cap: throws
bullet_band_cap: throws
word_cap: throws
violin_value_cap: throws
hierarchy_node_cap: throws
pie_long_label: yes
gauge_long_label: yes
bullet_long_label: yes
smooth_polyline: yes
smooth_valid_svg: yes
font_open_basedir: throws
