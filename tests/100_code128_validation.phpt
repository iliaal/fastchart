--TEST--
Code128: input validation + render-time encoder failures
--EXTENSIONS--
fastchart
--FILE--
<?php

// renderPng without setData throws Error (not ValueError) — the
// "no data set" guard fires before the encoder.
try {
    (new FastChart\Code128())->renderPng();
    echo "ERR: render without setData did not throw\n";
} catch (\Error $e) {
    echo "no-data: ", str_contains($e->getMessage(), 'setData') ? "ok" : $e->getMessage(), "\n";
}

// Oversized payload: setData itself accepts (only NUL is rejected at
// setData time); the size cap fires inside the encoder at render time.
try {
    (new FastChart\Code128())
        ->setData(str_repeat('A', 81))
        ->setSize(400, 80)
        ->renderPng();
    echo "ERR: oversized payload did not throw\n";
} catch (\ValueError $e) {
    echo "oversized: ", str_contains($e->getMessage(), 'max supported is 80') ? "ok" : $e->getMessage(), "\n";
}

// Non-ASCII byte (> 127) — Code 128 v0 does not support FNC4 extended
// ASCII. Setter accepts the bytes; encoder rejects at render time.
try {
    (new FastChart\Code128())
        ->setData("\xC3\xA9")  // UTF-8 'é', high byte 0xC3
        ->setSize(300, 80)
        ->renderPng();
    echo "ERR: non-ASCII payload did not throw\n";
} catch (\ValueError $e) {
    echo "non-ascii: ", str_contains($e->getMessage(), 'outside ASCII') ? "ok" : $e->getMessage(), "\n";
}

// Canvas too narrow for the encoded bars + default quiet zone.
// "1234567890" encodes to 5 codes + checksum + stop = 7 codes.
// Total modules = 6*11 + 13 = 79. With auto quiet zone (10 modules
// each side), needed total = 99 modules. At canvas width 50, module
// would round down to 0 → reject.
try {
    (new FastChart\Code128())
        ->setData('1234567890')
        ->setSize(50, 60)
        ->renderPng();
    echo "ERR: too-narrow canvas did not throw\n";
} catch (\ValueError $e) {
    echo "too-narrow: ", str_contains($e->getMessage(), 'too narrow') ? "ok" : $e->getMessage(), "\n";
}

// Quiet zone consumes the entire canvas.
try {
    (new FastChart\Code128())
        ->setData('123')
        ->setSize(100, 60)
        ->setQuietZone(60)  // 2 * 60 = 120 > 100
        ->renderPng();
    echo "ERR: quiet zone overrun did not throw\n";
} catch (\ValueError $e) {
    echo "qz-overrun: ", str_contains($e->getMessage(), 'quiet zone') ? "ok" : $e->getMessage(), "\n";
}

// renderToFile reuses the same encoder failure path.
$tmp = tempnam(sys_get_temp_dir(), 'fc-c128-bad-') . '.png';
try {
    (new FastChart\Code128())
        ->setData(str_repeat('A', 81))
        ->setSize(400, 80)
        ->renderToFile($tmp);
    echo "ERR: oversized renderToFile did not throw\n";
} catch (\ValueError $e) {
    echo "to-file-oversized: ", str_contains($e->getMessage(), 'max supported is 80') ? "ok" : $e->getMessage(), "\n";
}
@unlink($tmp);

?>
--EXPECT--
no-data: ok
oversized: ok
non-ascii: ok
too-narrow: ok
qz-overrun: ok
to-file-oversized: ok
