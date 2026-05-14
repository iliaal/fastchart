--TEST--
SVG_TEXT_NATIVE: C0 controls AND malformed UTF-8 are sanitized
--EXTENSIONS--
fastchart
--FILE--
<?php

/* XML 1.0 PCDATA is restrictive: it allows TAB (0x09), LF (0x0A),
 * CR (0x0D), and well-formed UTF-8 in the 0x20..0xD7FF /
 * 0xE000..0xFFFD / 0x10000..0x10FFFF code-point ranges. Any other
 * input makes a conforming XML parser reject the document.
 *
 * v1.0's first sanitizer pass only replaced single-byte C0 controls
 * and let bytes >= 0x80 through verbatim — which let ill-formed
 * UTF-8 (lone 0xC3, surrogate-pair-encoded U+D800, overlong forms,
 * trailing continuation bytes) escape into <text> content and break
 * downstream XML parsers. The hardened sanitizer validates UTF-8
 * byte sequences and replaces any malformed run with U+FFFD. */

function probe(string $title): array
{
    $svg = (new FastChart\LineChart(120, 80))
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setTitle($title)
        ->setSeries([1, 2, 3])
        ->renderSvg();
    $prior = libxml_use_internal_errors(true);
    libxml_clear_errors();
    $ok = @simplexml_load_string($svg) !== false;
    $err_count = count(libxml_get_errors());
    libxml_clear_errors();
    libxml_use_internal_errors($prior);
    return [$svg, $ok && $err_count === 0];
}

/* 1. C0 controls (the first pass already covered these). */
[$svg, $ok] = probe("hello\x01\x02\x08world");
echo "c0_xml_ok: ", ($ok ? "yes" : "no"), "\n";

/* 2. Lone continuation byte (0x80..0xBF without a leader). */
[$svg, $ok] = probe("oops\x80trail");
echo "lone_cont_xml_ok: ", ($ok ? "yes" : "no"), "\n";
echo "lone_cont_no_raw_80: ",
    (strpos($svg, "\x80") === false ? "yes" : "no"), "\n";

/* 3. Truncated 2-byte sequence (start byte without continuation). */
[$svg, $ok] = probe("xx\xC3yy");
echo "trunc2_xml_ok: ", ($ok ? "yes" : "no"), "\n";
echo "trunc2_no_raw_c3: ",
    (strpos($svg, "\xC3y") === false ? "yes" : "no"), "\n";

/* 4. Truncated 3-byte sequence. */
[$svg, $ok] = probe("xx\xE2\x82end");
echo "trunc3_xml_ok: ", ($ok ? "yes" : "no"), "\n";

/* 5. Surrogate encoded as UTF-8 (U+D800 -> ED A0 80). XML rejects
 * any surrogate code point in PCDATA. */
[$svg, $ok] = probe("xx\xED\xA0\x80end");
echo "surrogate_xml_ok: ", ($ok ? "yes" : "no"), "\n";

/* 6. Overlong-2 encoding of '/' (0x2F encoded as C0 AF). Invalid. */
[$svg, $ok] = probe("xx\xC0\xAFend");
echo "overlong_xml_ok: ", ($ok ? "yes" : "no"), "\n";

/* 7. Code point above U+10FFFF (5-byte start 0xF8+). */
[$svg, $ok] = probe("xx\xF8\x88\x80\x80\x80end");
echo "above_max_xml_ok: ", ($ok ? "yes" : "no"), "\n";

/* 8. Valid multi-byte UTF-8 must round-trip untouched. */
$valid = "Ωμέγα — €100 漢字";  /* 2-byte, 3-byte, 3-byte mix */
[$svg, $ok] = probe($valid);
echo "valid_utf8_xml_ok: ", ($ok ? "yes" : "no"), "\n";
echo "valid_utf8_preserved: ",
    (str_contains($svg, "Ωμέγα") && str_contains($svg, "漢字")
        ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
c0_xml_ok: yes
lone_cont_xml_ok: yes
lone_cont_no_raw_80: yes
trunc2_xml_ok: yes
trunc2_no_raw_c3: yes
trunc3_xml_ok: yes
surrogate_xml_ok: yes
overlong_xml_ok: yes
above_max_xml_ok: yes
valid_utf8_xml_ok: yes
valid_utf8_preserved: yes
ok
