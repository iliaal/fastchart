--TEST--
Source-image dimension cap uses the decoder's IHDR, not bytes inside a leading CgBI chunk
--EXTENSIONS--
fastchart
--FILE--
<?php
/* stb_image skips a leading Apple CgBI chunk and decodes the IHDR that
 * follows it. A loader that reads width/height at fixed offset 16 sees
 * the CgBI payload instead, so a decoy 1x1 there let an over-cap IHDR
 * through to a decode outside memory_limit. 4097 is one past the
 * 4096-per-side cap and keeps the over-cap decode cheap. */

function chunk(string $t, string $d): string
{
    return pack('N', strlen($d)) . $t . $d . pack('N', crc32($t . $d));
}

/* Single stored deflate block; every payload here is under 64 KiB. */
function stored(string $raw): string
{
    return "\x01" . pack('vv', strlen($raw), 0xFFFF ^ strlen($raw)) . $raw;
}

function png(int $w, int $h, ?array $decoy = null): string
{
    $raw = str_repeat("\x00" . str_repeat("\x30\xc0\x60\xff", $w), $h);
    $out = "\x89PNG\r\n\x1a\n";
    if ($decoy !== null) {
        /* CgBI streams are raw deflate without the zlib wrapper. */
        $out .= chunk('CgBI', pack('NN', $decoy[0], $decoy[1])
            . "\x50\x00\x20\x06\x00");
        $idat = stored($raw);
    } else {
        $idat = "\x78\x01" . stored($raw)
            . pack('H*', hash('adler32', $raw));
    }
    return $out . chunk('IHDR', pack('NNCCCCC', $w, $h, 8, 6, 0, 0, 0))
        . chunk('IDAT', $idat) . chunk('IEND', '');
}

$dir = sys_get_temp_dir();
$cases = [
    'plain_2x2'         => png(2, 2),
    'plain_4097x1'      => png(4097, 1),
    'cgbi_decoy_4097x1' => png(4097, 1, [1, 1]),
    'cgbi_decoy_1x4097' => png(1, 4097, [1, 1]),
    'cgbi_2x2'          => png(2, 2, [2, 2]),
];
$paths = [];
foreach ($cases as $name => $bytes) {
    $paths[$name] = "$dir/fc_510_$name.png";
    file_put_contents($paths[$name], $bytes);
}

function chart(): FastChart\LineChart
{
    return (new FastChart\LineChart(200, 120))->setSeries([1, 2, 3]);
}

foreach ($paths as $name => $p) {
    $bg = chart()->setBackgroundImage($p)->renderSvg();
    $icon = chart()->addIconAt(1, 2, $p, 40, 40)->renderSvg();
    printf("%-18s bg=%s icon=%s\n", $name,
        str_contains($bg, '<image') ? 'embedded' : 'rejected',
        str_contains($icon, '<image') ? 'embedded' : 'rejected');
}

/* Raster path shares the loader: the over-cap CgBI image must leave the
 * PNG byte-identical to a chart without any image. */
$none = chart()->renderPng();
$bg = chart()->setBackgroundImage($paths['cgbi_decoy_4097x1'])->renderPng();
$icon = chart()->addIconAt(1, 2, $paths['cgbi_decoy_4097x1'], 40, 40)
    ->renderPng();
echo "raster bg skipped: ", $bg === $none ? 'yes' : 'no', "\n";
echo "raster icon skipped: ", $icon === $none ? 'yes' : 'no', "\n";
$plain = chart()->setBackgroundImage($paths['plain_2x2'])->renderPng();
echo "raster plain used: ", $plain !== $none ? 'yes' : 'no', "\n";

foreach ($paths as $p) {
    @unlink($p);
}
?>
--EXPECT--
plain_2x2          bg=embedded icon=embedded
plain_4097x1       bg=rejected icon=rejected
cgbi_decoy_4097x1  bg=rejected icon=rejected
cgbi_decoy_1x4097  bg=rejected icon=rejected
cgbi_2x2           bg=rejected icon=rejected
raster bg skipped: yes
raster icon skipped: yes
raster plain used: yes
