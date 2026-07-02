--TEST--
Text: malformed UTF-8 (bad continuation, overlong, truncated) renders safely
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* Regression: the shared UTF-8 walker consumed any following byte as a
 * continuation once a lead byte matched, folding e.g. 0xC2 + 'A' into one
 * bogus codepoint and swallowing the ASCII char. It now validates each
 * continuation byte (10xxxxxx) and rejects overlong/surrogate/oversized
 * sequences, substituting U+FFFD and advancing one byte. All of these
 * malformed labels must still produce valid, complete output. */

function render(string $title): string {
    return (new FastChart\BarChart(300, 200))
        ->setTitle($title)
        ->setCategoryLabels(['x'])
        ->setSeries([['name' => 's', 'data' => [1]]])
        ->renderSvg();
}
function ok(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$cases = [
    'lead_plus_ascii' => "\xC2A B",       /* 2-byte lead then ASCII */
    'truncated_2byte' => "x\xC2",          /* lead byte at end */
    'overlong_slash'  => "\xC0\xAF",       /* overlong encoding of '/' */
    'bare_continuation' => "\x80\x80",     /* stray continuation bytes */
    'bad_3byte'       => "\xE2\x28\xA1",   /* invalid second byte */
    'valid_multibyte' => "caf\xC3\xA9",    /* proper UTF-8 "café" */
];

foreach ($cases as $name => $title) {
    echo $name, ": ", (ok(render($title)) ? "ok" : "BAD"), "\n";
}

?>
--EXPECT--
lead_plus_ascii: ok
truncated_2byte: ok
overlong_slash: ok
bare_continuation: ok
bad_3byte: ok
valid_multibyte: ok
