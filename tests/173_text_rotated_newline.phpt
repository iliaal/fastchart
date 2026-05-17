--TEST--
fastchart_text_draw_rotated: \n in rotated text splits into multiple <text> elements
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: fastchart_text_draw_rotated at fastchart_text.c:188
 * called fc_svg_emit_text once on the full text including any \n
 * bytes, producing a single <text> element with raw \n inside it.
 * SVG <text> collapses newlines to whitespace at render time, so
 * a multi-line rotated label rendered as one squashed line. The
 * companion measure() function special-cases \n and reports a
 * multi-line height, so layout reservation and rendering
 * disagreed. */

$svg = (new FastChart\LineChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([['data' => [1, 2, 3]]])
    ->setYAxisTitle("First\nSecond")
    ->renderSvg();

/* Count <text> elements rotated -90° (the y-axis title is the only
 * such element in a default LineChart). With FIX: 2 elements (one
 * per line). With BUG: 1 element with embedded "First\nSecond". */
$n_rotated = preg_match_all('#<text[^>]*transform="rotate\(-90 [^>]*>#', $svg, $m);

echo "rotated_text_count: ",
    ($n_rotated === 2 ? "ok" : "BAD ($n_rotated)"), "\n";

/* Each <text> element body should contain ONE of the lines, not
 * both with a raw \n between them. */
$has_first_only  = false;
$has_second_only = false;
if (preg_match_all('#<text[^>]*transform="rotate\(-90 [^>]*>([^<]*)</text>#', $svg, $bodies)) {
    foreach ($bodies[1] as $body) {
        if ($body === 'First')  $has_first_only  = true;
        if ($body === 'Second') $has_second_only = true;
    }
}
echo "first_line_in_own_text:  ", ($has_first_only  ? "ok" : "BAD"), "\n";
echo "second_line_in_own_text: ", ($has_second_only ? "ok" : "BAD"), "\n";

echo "done\n";
?>
--EXPECT--
rotated_text_count: ok
first_line_in_own_text:  ok
second_line_in_own_text: ok
done
