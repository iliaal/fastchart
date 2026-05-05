--TEST--
GanttChart: timeline bars + milestones + dependency arrows
--EXTENSIONS--
fastchart
--FILE--
<?php

$bytes = (new FastChart\GanttChart(700, 400))
    ->setTitle('Project plan')
    ->setTasks([
        ['name' => 'Plan',  'start' => 1700000000, 'end' => 1700432000],
        ['name' => 'Build', 'start' => 1700432000, 'end' => 1701036800, 'depends' => [0]],
        ['name' => 'Test',  'start' => 1700864000, 'end' => 1701123200, 'depends' => [1], 'color' => 0xFF8800],
        ['name' => 'Ship',  'start' => 1701123200, 'end' => 1701123200, 'milestone' => true, 'depends' => [2]],
    ])
    ->renderPng();
echo "renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";

$im = imagecreatefromstring($bytes);
$has = function ($im, $c, $w, $h) {
    for ($y = 0; $y < $h; $y++)
        for ($x = 0; $x < $w; $x++)
            if (imagecolorat($im, $x, $y) === $c) return true;
    return false;
};
echo "task_color: ", $has($im, 0xFF8800, 700, 400) ? "yes" : "no", "\n";

// Forced range.
$bytes2 = (new FastChart\GanttChart(700, 400))
    ->setTasks([['name' => 'A', 'start' => 1700100000, 'end' => 1700200000]])
    ->setTimeRange(1700000000, 1700500000)
    ->renderPng();
echo "forced_range: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";

// Bad range.
try {
    (new FastChart\GanttChart)->setTimeRange(1000, 500);
    echo "bad_range: no throw\n";
} catch (\ValueError $e) {
    echo "bad_range: ValueError ok\n";
}

// Empty data.
try {
    (new FastChart\GanttChart(400, 200))->setTasks([])->renderPng();
    echo "empty: no throw\n";
} catch (\Error $e) {
    echo "empty: error ok\n";
}

// Hide labels.
$bytes3 = (new FastChart\GanttChart(700, 400))
    ->setTasks([['name' => 'A', 'start' => 1700000000, 'end' => 1700100000]])
    ->setShowTaskLabels(false)
    ->renderPng();
echo "no_labels: ", strlen($bytes3) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
renders: ok
task_color: yes
forced_range: ok
bad_range: ValueError ok
empty: error ok
no_labels: ok
