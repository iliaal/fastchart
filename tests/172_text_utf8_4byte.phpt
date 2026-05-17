--TEST--
fc_ft_measure: 4-byte UTF-8 sequences (emoji, non-BMP) counted in measured width
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: fc_ft_measure() at fastchart_text.c:54-61 recognised
 * only 1-, 2-, and 3-byte UTF-8 leads. A 4-byte lead (0xF0-0xF7,
 * covering U+10000+ — emoji, CJK extensions, math symbols) fell into
 * the else { p++; continue; } and each of the three continuation
 * bytes was then skipped (no branch matched). Result: every non-BMP
 * codepoint contributed 0 to the measured width.
 *
 * Observable signal: the y-axis title's rendered y coordinate is
 * computed as `y = cy + tw / 2` where tw is the measured width.
 * So adding a 4-byte codepoint to the title text should INCREASE
 * the emitted y, by approximately one glyph's advance / 2. With
 * the bug, the emoji contributes 0 so tw stays the same as the
 * baseline string. */

$base = (new FastChart\LineChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([['data' => [1, 2, 3]]])
    ->setYAxisTitle("ABCDE")
    ->renderSvg();

$with_emoji = (new FastChart\LineChart(400, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([['data' => [1, 2, 3]]])
    ->setYAxisTitle("AB\xF0\x9F\x8E\x89CDE")  /* 🎉 in the middle */
    ->renderSvg();

/* Find the y-axis title <text> element. It's the one rotated 90°
 * via transform="rotate(-90 ...)". Pull its y attribute. */
$extract_title_y = function($svg) {
    if (!preg_match_all('#<text x="\d+(?:\.\d+)?" y="(\d+(?:\.\d+)?)"[^>]*transform="rotate\(-90 #', $svg, $m)) {
        return null;
    }
    /* Return the y of the FIRST rotated text (only the y-axis title
     * gets a transform with -90 + extras in this chart). */
    return (float)$m[1][0];
};

$y_base  = $extract_title_y($base);
$y_emoji = $extract_title_y($with_emoji);

if ($y_base === null || $y_emoji === null) {
    echo "no_rotated_text_found: REGRESSION (y-axis title missing)\n";
    exit;
}

/* With the FIX: emoji adds an advance to tw, so y_emoji > y_base
 * by at least a few pixels. With the BUG: emoji contributes 0 to
 * tw, so y_emoji ≈ y_base. */
$delta = $y_emoji - $y_base;
echo "emoji_increases_measured_width: ",
    ($delta >= 3.0 ? "ok" : "BAD (delta=$delta)"), "\n";

echo "done\n";
?>
--EXPECT--
emoji_increases_measured_width: ok
done
