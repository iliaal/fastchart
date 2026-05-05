<?php
/* GanttChart: tasks with [name, start, end, depends?, milestone?]
 * shapes. Milestones render as diamonds at a single point.
 * setShowTaskLabels off would hide the per-row text. */

require __DIR__ . '/_bootstrap.php';

$base = 1700000000;
$day  = 86400;

(new FastChart\GanttChart(720, 380))
    ->setFontPath($font)
    ->setTitle('Q4 release timeline')
    ->setTimeRange($base, $base + 60 * $day)
    ->setTasks([
        ['name' => 'Spec',         'start' => $base + 0  * $day, 'end' => $base + 7  * $day, 'color' => 0x4F86C6],
        ['name' => 'Design',       'start' => $base + 5  * $day, 'end' => $base + 18 * $day, 'depends' => [0]],
        ['name' => 'Build',        'start' => $base + 15 * $day, 'end' => $base + 38 * $day, 'depends' => [1]],
        ['name' => 'QA',           'start' => $base + 35 * $day, 'end' => $base + 50 * $day, 'depends' => [2]],
        ['name' => 'Launch',       'start' => $base + 50 * $day, 'end' => $base + 50 * $day,
         'milestone' => true, 'color' => 0xE34A6F],
    ])
    ->setShowTaskLabels(true)
    ->renderToFile(__DIR__ . '/17_gantt.png');
