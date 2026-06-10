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
    if (strpos($svg, 'good') === false || strpos($svg, 'also-good') === false) {
        echo "MISSING good task label(s)\n"; $ok = false;
    }
    if (strpos($svg, 'bad1') !== false || strpos($svg, 'bad2') !== false) {
        echo "BAD task(s) should have been omitted\n"; $ok = false;
    }
    echo "gantt_best_effort: ", $ok ? "ok\n" : "FAIL\n";
} catch (Throwable $e) {
    echo "unexpected_throw: ", get_class($e), ": ", $e->getMessage(), "\n";
}
?>
--EXPECT--
gantt_best_effort: ok
