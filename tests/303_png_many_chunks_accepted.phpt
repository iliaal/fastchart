--TEST--
Background image: a spec-legal PNG with >1024 IDAT chunks is accepted
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
?>
--INI--
asan.detect_leaks=0
--FILE--
<?php
/* fc_validate_png_chunks capped the chunk walk at 1024 iterations and
 * rejected anything longer into a silent solid-fill fallback. libpng
 * emits ~8KB IDATs, and a valid file near the 8MB size cap can hold far
 * more than 1023 chunks; the PNG spec explicitly permits IDAT to be
 * split across any number of chunks. Build such a file by re-splitting a
 * fastchart-rendered PNG's IDAT stream into one-byte IDAT chunks (each
 * with a correct CRC), then confirm it is embedded rather than dropped. */

require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();

$png = (new FastChart\BarChart(120, 90))
    ->setSeries([["label" => "v", "data" => [3, 7, 4, 9, 2, 6]]])
    ->setFontPath($font)
    ->renderPng();

/* Split the source PNG into signature + IHDR + concatenated IDAT + IEND. */
$sig = substr($png, 0, 8);
$off = 8; $ihdr = ""; $idat = ""; $iend = "";
while ($off + 12 <= strlen($png)) {
    $len  = unpack("N", substr($png, $off, 4))[1];
    $type = substr($png, $off + 4, 4);
    if ($type === "IHDR")      $ihdr  = substr($png, $off, 12 + $len);
    elseif ($type === "IDAT")  $idat .= substr($png, $off + 8, $len);
    elseif ($type === "IEND") { $iend = substr($png, $off, 12 + $len); break; }
    $off += 12 + $len;
}

function png_chunk(string $type, string $data): string {
    return pack("N", strlen($data)) . $type . $data
         . pack("N", crc32($type . $data));
}

/* One IDAT chunk per byte: 2500-ish bytes -> 2500 chunks, well over the
 * old 1024 cap. The concatenated IDAT payload is unchanged, so the file
 * decodes identically. */
$body = "";
$nchunks = 0;
for ($i = 0; $i < strlen($idat); $i++) {
    $body .= png_chunk("IDAT", $idat[$i]);
    $nchunks++;
}
$rebuilt = $sig . $ihdr . $body . $iend;

$path = sys_get_temp_dir() . "/fc_303_" . getmypid() . ".png";
file_put_contents($path, $rebuilt);

$svg = (new FastChart\BarChart(200, 150))
    ->setSeries([["label" => "v", "data" => [1, 2, 3]]])
    ->setFontPath($font)
    ->setBackgroundImage($path)
    ->renderSvg();
@unlink($path);

echo "chunk_count_over_1024: ", ($nchunks > 1024 ? "yes" : "no"), "\n";
echo "many_chunk_png_used: ",
    (str_contains($svg, "data:image/png") ? "yes" : "no"), "\n";
?>
--EXPECT--
chunk_count_over_1024: yes
many_chunk_png_used: yes
