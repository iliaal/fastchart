--TEST--
SVG_TEXT_NATIVE: XML-invalid control bytes are sanitized in output
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The XML 1.0 spec allows TAB (0x09), LF (0x0A), CR (0x0D), and
 * 0x20+ in PCDATA. Other C0 control bytes (0x01-0x08, 0x0B, 0x0C,
 * 0x0E-0x1F) make a conforming XML parser reject the document.
 *
 * fc_svg_escape() previously escaped only the five XML metacharacters
 * and passed every other byte through verbatim, so user-supplied
 * titles / labels containing control bytes produced invalid XML
 * under SVG_TEXT_NATIVE. The sanitizer now replaces those bytes with
 * U+FFFD (UTF-8 EF BF BD). */

$probes = "head\x01ing\x02 \x08 \x0b \x0c \x0e \x1f tail";

$svg = (new FastChart\LineChart(200, 100))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setTitle($probes)
    ->setSeries([1, 2, 3])
    ->renderSvg();

/* No raw C0 control bytes survive (except TAB/LF/CR, which are
 * allowed and may appear in pretty-printed structure). */
$invalid = [0x01, 0x02, 0x08, 0x0B, 0x0C, 0x0E, 0x1F];
foreach ($invalid as $b) {
    $hex = sprintf('0x%02X', $b);
    echo "byte $hex absent: ",
        (strpos($svg, chr($b)) === false ? "yes" : "no"), "\n";
}

/* Every invalid byte we sent in became a U+FFFD. */
echo "fffd_count: ", substr_count($svg, "\xEF\xBF\xBD"), "\n";

/* SimpleXMLElement performs strict XML validation — invalid bytes
 * would raise a warning here. We capture warnings instead of
 * silencing them so a regression resurfaces visibly. */
$prior = libxml_use_internal_errors(true);
libxml_clear_errors();
$ok = @simplexml_load_string($svg) !== false;
$err_count = count(libxml_get_errors());
libxml_clear_errors();
libxml_use_internal_errors($prior);
echo "xml_parse_ok: ", ($ok && $err_count === 0 ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
byte 0x01 absent: yes
byte 0x02 absent: yes
byte 0x08 absent: yes
byte 0x0B absent: yes
byte 0x0C absent: yes
byte 0x0E absent: yes
byte 0x1F absent: yes
fffd_count: 7
xml_parse_ok: yes
ok
