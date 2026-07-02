--TEST--
LinearMeter: a high-precision value format is not silently truncated
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* Regression: min/max/value labels were formatted into fixed 32/64-byte
 * stack buffers, but the accepted format grammar allows up to three
 * digits of precision ("%.999f"). snprintf truncated the label silently.
 * Labels are now formatted through a right-sized heap buffer. */

$svg = (new FastChart\LinearMeter(400, 120))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setRange(0, 100)
    ->setValue(50)
    ->setValueFormat('%.300f')
    ->renderSvg();

echo "valid_svg: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false ? "yes" : "no"),
    "\n";
/* 50 with %.300f is one leading digit + '.' + 300 fraction digits; the
 * old 64-byte buffer capped this near 63 chars. A run of >= 250 fraction
 * digits proves the label survived intact. */
echo "full_precision: ", (preg_match('/\.[0-9]{250,}/', $svg) ? "yes" : "no"), "\n";

?>
--EXPECT--
valid_svg: yes
full_precision: yes
