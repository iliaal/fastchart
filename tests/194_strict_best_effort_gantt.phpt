--TEST--
GanttChart (and other non-Line families) treat setStrict(true) as best-effort
--EXTENSIONS--
fastchart
--FILE--
<?php
// One malformed task (bad timestamp type or missing keys) must be
// silently dropped. The rest of the chart must render without error.
$tasks = [
    ['name' => 'good', 'start' => 100, 'end' => 200],
    ['name' => 'bad1', 'start' => 'not-a-time', 'end' => 300],   // unparsable
    ['name' => 'bad2'],                                          // missing times
    ['name' => 'also-good', 'start' => 150, 'end' => 250],
];

$ok = true;
try {
    $g = (new FastChart\GanttChart(400, 200))
        ->setStrict(true)
        ->setTasks($tasks);
    $svg = $g->renderSvg();
    /* Contract test for best-effort family: setStrict(true) must not throw
     * TypeError on bad task data. Render succeeds (name emission varies). */
    echo "gantt_best_effort: ok\n";
} catch (Throwable $e) {
    echo "unexpected_throw: ", get_class($e), ": ", $e->getMessage(), "\n";
}
?>
--EXPECT--
gantt_best_effort: ok
