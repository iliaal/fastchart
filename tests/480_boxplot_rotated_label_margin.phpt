--TEST--
BoxPlot: layout measures resolved box labels without changing category labels
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
$svg = (new FastChart\BoxPlot(640, 480))
    ->setBoxes([[1, 2, 3, 4, 5]])
    ->setCategoryLabels(['LabelProbe'])
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->renderSvg();
if (!str_contains($svg, '>LabelProbe</text>')) {
    die('skip no usable default font');
}
?>
--FILE--
<?php
use FastChart\BoxPlot;
use FastChart\Chart;

function boxes(array $labels): array
{
    return array_map(static fn($label) => [
        'min' => 1, 'q1' => 2, 'median' => 3, 'q3' => 4, 'max' => 5,
        'label' => $label,
    ], $labels);
}

function chart(array $labels, array $categories, int $angle, int $mode): BoxPlot
{
    return (new BoxPlot(640, 480))
        ->setBoxes(boxes($labels))
        ->setCategoryLabels($categories)
        ->setXAxisLabelAngle($angle)
        ->setSvgTextMode($mode);
}

$labels = ['First group', str_repeat('Long group ', 4)];
foreach ([0, 45, 90] as $angle) {
    foreach (['native' => Chart::SVG_TEXT_NATIVE, 'paths' => Chart::SVG_TEXT_PATHS] as $name => $mode) {
        echo "$angle $name\n";
        $fallback = chart($labels, [], $angle, $mode);
        $svg = $fallback->renderSvg();
        /* Supplying the same labels through either API must produce the
         * same geometry, including the reserved rotated-label margin. */
        var_dump($svg === chart($labels, $labels, $angle, $mode)->renderSvg());
        var_dump($svg === $fallback->renderSvg());

        /* A short category list and a null category slot both fall back
         * to the per-box label for the remaining entry. */
        $resolved = ['Override', $labels[1]];
        $expected = chart($labels, $resolved, $angle, $mode)->renderSvg();
        $partial = chart($labels, ['Override'], $angle, $mode);
        var_dump($expected === $partial->renderSvg());
        var_dump($expected === chart($labels, ['Override', null], $angle, $mode)->renderSvg());

        /* Unused category labels must not reserve extra bottom margin. */
        $categories = [...$resolved, str_repeat('Unused ', 12)];
        $surplus = chart($labels, $categories, $angle, $mode);
        var_dump($expected === $surplus->renderSvg());
        /* The original count also survives: a new box can use that label. */
        $moreLabels = [...$labels, 'Third'];
        $surplus->setBoxes(boxes($moreLabels));
        var_dump($surplus->renderSvg() === chart($moreLabels, $categories, $angle, $mode)->renderSvg());

        /* A long label skipped by the explicit stride needs no margin. */
        var_dump(chart($labels, [], $angle, $mode)->setXLabelStride(2)->renderSvg()
            === chart([$labels[0], 'Short'], [], $angle, $mode)->setXLabelStride(2)->renderSvg());

        /* Rendering must restore the original category array: replacing
         * boxes still resolves the missing label from the new box data. */
        $changed = ['New first', str_repeat('Replacement ', 2)];
        $partial->setBoxes(boxes($changed));
        $expected = chart($changed, ['Override', $changed[1]], $angle, $mode)->renderSvg();
        var_dump($expected === $partial->renderSvg());
        var_dump($expected === (clone $partial)->renderSvg());
    }
}
?>
--EXPECT--
0 native
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
0 paths
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
45 native
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
45 paths
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
90 native
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
90 paths
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
