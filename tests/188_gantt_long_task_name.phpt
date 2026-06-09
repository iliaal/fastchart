--TEST--
GanttChart: a task name wider than the plot must not invert the time axis
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') echo "skip no system font present\n";
?>
--FILE--
<?php

require __DIR__ . '/_font_candidates.inc';

/* The label margin is measured from the widest task name. Pre-fix a
 * name wider than the plot pushed bars.x0 past bars.x1, giving the
 * time→pixel mapping a negative width: bars, ticks, and labels were
 * emitted at x positions far beyond the canvas. Post-fix the margin
 * is clamped and everything stays inside the viewport. */

$W = 400;
$c = (new FastChart\GanttChart($W, 300))
    ->setFontPath(fc_pick_font())
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setTasks([
        ['name' => str_repeat('Very long task name segment ', 10),
         'start' => 1700000000, 'end' => 1700345600],
        ['name' => 'short',
         'start' => 1700086400, 'end' => 1700259200],
    ]);

$svg = $c->renderSvg();

preg_match_all('/<(?:rect|text|line)\b[^>]*\bx1?="(-?[0-9.]+)"/', $svg, $m);
$max_x = max(array_map(floatval(...), $m[1]));
var_dump($max_x <= $W + 50);

/* Sanity: the render still contains task bars (rects beyond the
 * background + frame). */
var_dump(substr_count($svg, '<rect') >= 3);

?>
--EXPECT--
bool(true)
bool(true)
