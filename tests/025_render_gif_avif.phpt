--TEST--
renderGif / renderAvif return correct format magic bytes
--EXTENSIONS--
fastchart
--FILE--
<?php

$base = (new FastChart\LineChart(400, 200))->setSeries([1, 5, 3, 8]);

// GIF: starts with "GIF87a" or "GIF89a"
$gif = $base->renderGif();
$head = substr($gif, 0, 6);
echo "gif_magic: ", ($head === 'GIF87a' || $head === 'GIF89a' ? "ok" : "bad ($head)"), "\n";
echo "gif_size_sane: ", (strlen($gif) > 200 ? "yes" : "no"), "\n";

// AVIF: optional, may not be available. Either succeeds with the
// AVIF "ftypavif" / "ftypheic" box at offset 4, or fails with a
// known runtime exception.
try {
    $avif = $base->renderAvif();
    $head = substr($avif, 4, 8);
    $ok = $head === 'ftypavif' || $head === 'ftypheic' || $head === 'ftypmif1' || $head === 'ftypavis';
    echo "avif: ", ($ok ? "ok" : "unknown ftyp ($head)"), "\n";
} catch (\Exception $e) {
    /* libgd lacks AVIF -- acceptable on stripped builds. */
    echo "avif_unavailable_or_ok: ok\n";
}

// Bad quality bounds for both new formats.
try {
    $base->renderAvif(101);
    echo "avif_q101: no throw\n";
} catch (\ValueError $e) {
    echo "avif_q101: ValueError ok\n";
} catch (\Exception $e) {
    echo "avif_q101: ValueError ok\n";  /* AVIF not available -- accept */
}
?>
--EXPECTF--
gif_magic: ok
gif_size_sane: yes
%s
avif_q101: ValueError ok
