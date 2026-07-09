--TEST--
Nested per-entry count caps reject over-cap input without leaking committed rows
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Each case commits one valid parent row (with heap-owned label and
 * child array) and then feeds a second row whose nested child count
 * exceeds the per-parent cap. The setter must throw ValueError and
 * free the already-committed row; the leak-checked CI lane is what
 * proves the cleanup path is complete. */

function value_error(string $label, callable $fn): void {
    try {
        $fn();
        echo "$label: NO THROW\n";
    } catch (ValueError $e) {
        echo "$label: throws\n";
    }
}

value_error('gantt_dep_cap', fn() => (new FastChart\GanttChart(400, 200))->setTasks([
    ['name' => 'first', 'start' => 0, 'end' => 5, 'depends' => [0]],
    ['name' => 'second', 'start' => 6, 'end' => 9, 'depends' => array_fill(0, 65, 0)],
]));

value_error('boxplot_outlier_cap', fn() => (new FastChart\BoxPlot(400, 200))->setBoxes([
    ['label' => 'first', 'min' => 0, 'q1' => 1, 'median' => 2, 'q3' => 3, 'max' => 4,
        'outliers' => [10, 20]],
    ['label' => 'second', 'min' => 0, 'q1' => 1, 'median' => 2, 'q3' => 3, 'max' => 4,
        'outliers' => array_fill(0, 129, 99.0)],
]));

value_error('marimekko_segment_cap', fn() => (new FastChart\MarimekkoChart(400, 200))->setColumns([
    ['label' => 'first', 'segments' => [['label' => 's1', 'value' => 1], ['label' => 's2', 'value' => 2]]],
    ['label' => 'second', 'segments' => array_fill(0, 65, ['label' => 'seg', 'value' => 1])],
]));

?>
--EXPECT--
gantt_dep_cap: throws
boxplot_outlier_cap: throws
marimekko_segment_cap: throws
