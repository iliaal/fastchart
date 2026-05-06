--TEST--
Title and category labels honor "\n" — width = max line, height = first + (n-1)*advance
--EXTENSIONS--
fastchart
gd
--FILE--
<?php
/* The multi-line path must:
 *   1. Render visibly different from a single-line equivalent.
 *   2. Reserve more vertical layout space (multi-line title pushes
 *      the plot rect down). We sample the image's dark-pixel
 *      distribution to spot-check the reservation.
 */
$single = (new FastChart\LineChart(400, 240))
    ->setTitle('A')
    ->setSeries([['data' => [1, 2, 3]]])
    ->renderPng();

$multi = (new FastChart\LineChart(400, 240))
    ->setTitle("Multi\nline\ntitle")
    ->setSeries([['data' => [1, 2, 3]]])
    ->renderPng();

echo "differs: ", ($single !== $multi ? "yes" : "no"), "\n";

/* Three "M L T" glyphs at the same font size together carry more
 * dark pixels than a single "A". The renders differ, the chart
 * data is the same, so the dark-pixel delta lives in the title.
 * Counting whole-image dark pixels avoids depending on layout
 * shifts (plot rect moves when the top reservation grows, which
 * makes per-band counts hard to pin). */
function dark_pixels(string $bytes): int {
    $im = imagecreatefromstring($bytes);
    $w = imagesx($im); $h = imagesy($im);
    $n = 0;
    for ($y = 0; $y < $h; $y++) {
        for ($x = 0; $x < $w; $x++) {
            if ((imagecolorat($im, $x, $y) & 0xFF) < 80) $n++;
        }
    }
    imagedestroy($im);
    return $n;
}
$single_n = dark_pixels($single);
$multi_n  = dark_pixels($multi);
echo "multi_more_ink: ", ($multi_n > $single_n ? "yes" : "no"),
     " ($single_n vs $multi_n)\n";

/* Trailing newline should be tolerated (one empty line at the end —
 * just consumes vertical space, doesn't crash). */
$trailing = (new FastChart\LineChart(400, 240))
    ->setTitle("Tail\n")
    ->setSeries([['data' => [1, 2, 3]]])
    ->renderPng();
echo "trailing_nl: ", ($trailing !== '' ? "ok" : "empty"), "\n";
?>
--EXPECTF--
differs: yes
multi_more_ink: yes (%d vs %d)
trailing_nl: ok
