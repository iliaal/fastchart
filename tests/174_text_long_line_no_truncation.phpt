--TEST--
fastchart_text_draw / _measure: text lines longer than 1023 bytes are NOT truncated
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: fastchart_text.c:138/144 and 228/233 used a fixed
 * `char buf[1024]` and silently capped any line at sizeof(buf)-1
 * bytes. The cap was in bytes, not codepoints; the truncation
 * could split a multi-byte UTF-8 sequence and produce malformed
 * XML. It also silently dropped any line content past byte 1023. */

/* 1504 ASCII chars (well past the buggy 1023 cap). */
$long = str_repeat("0123456789ABCDEF", (int)(1500 / 16));
echo "input_len: ", strlen($long), "\n";

$svg = (new FastChart\LineChart(2000, 200))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setSeries([['data' => [1, 2, 3]]])
    ->addTextAnnotation($long, 10, 100)
    ->renderSvg();

/* Extract the annotation's <text> body. Annotations emit with
 * text-anchor implicit (left), no transform. The body is the
 * long 1500-char string. */
if (!preg_match_all('#<text[^>]*>([^<]*)</text>#', $svg, $m)) {
    echo "no_text_elements: REGRESSION\n";
    exit;
}

/* Find the longest body — that's our annotation. */
$longest = '';
foreach ($m[1] as $body) {
    if (strlen($body) > strlen($longest)) $longest = $body;
}
echo "emitted_body_len: ", strlen($longest), "\n";

/* With the FIX: full 1500 chars in the SVG. With the BUG: 1023
 * chars (sizeof(buf)-1). */
echo "no_truncation: ",
    (strlen($longest) === strlen($long) ? "ok" : "BAD"), "\n";

echo "done\n";
?>
--EXPECT--
input_len: 1488
emitted_body_len: 1488
no_truncation: ok
done
