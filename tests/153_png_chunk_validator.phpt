--TEST--
PNG chunk-length pre-scan rejects falsified IDAT length amplifier
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Defends against stb_image's IDAT chunk-length amplifier: a 30-byte
 * PNG with a falsified IDAT chunk length of 0x3FFFFFFF causes
 * vendored stb_image to attempt a ~1 GB realloc before reading any
 * payload — single-request OOM-kill on memory-constrained workers.
 * fastchart_target.c::fc_validate_png_chunks walks the chunk list
 * and rejects any declared length that overruns the buffer. */

$tmpdir = sys_get_temp_dir();

/* Legit baseline: minimal valid 1x1 grayscale PNG. */
$sig = "\x89PNG\r\n\x1a\n";
$ihdr_data = pack("NNCCCCC", 1, 1, 8, 0, 0, 0, 0);
$ihdr = pack("N", 13) . "IHDR" . $ihdr_data
      . pack("N", crc32("IHDR" . $ihdr_data));
/* IDAT containing a real (if minimal) zlib-compressed 2-byte stream
 * (filter=0 + pixel=0xFF). The CRC isn't validated by the pre-scan
 * but stb_image expects sane chunk framing. */
$idat_data = "\x78\x9c\x63\xfa\x0f\x00\x01\x02\x01\x00";
$idat_real = pack("N", strlen($idat_data)) . "IDAT" . $idat_data
           . pack("N", crc32("IDAT" . $idat_data));
$iend = pack("N", 0) . "IEND" . pack("N", crc32("IEND"));
$legit = $tmpdir . "/fc_validator_legit.png";
file_put_contents($legit, $sig . $ihdr . $idat_real . $iend);

/* Attack payload: identical structure but IDAT declares 0x3FFFFFFF
 * bytes of payload while supplying none. */
$idat_lie = pack("N", 0x3FFFFFFF) . "IDAT" . pack("N", 0);
$mal_path = $tmpdir . "/fc_validator_mal.png";
file_put_contents($mal_path, $sig . $ihdr . $idat_lie . $iend);

foreach ([$legit, $mal_path] as $p) {
    $c = (new FastChart\LineChart())
        ->setSize(200, 100)
        ->setSeries([1, 2, 3])
        ->setBackgroundImage($p);
    $svg = $c->renderSvg();
    $tag  = basename($p);
    $has  = strpos($svg, "<image") !== false ? "embedded" : "rejected";
    echo "$tag: $has\n";
}

@unlink($legit);
@unlink($mal_path);
?>
--EXPECT--
fc_validator_legit.png: embedded
fc_validator_mal.png: rejected
