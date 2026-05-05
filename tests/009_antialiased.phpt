--TEST--
Antialiased line segments emit blended pixels along diagonals
--EXTENSIONS--
fastchart
--FILE--
<?php

// A line chart with sharp diagonal moves between adjacent points
// produces antialiased edge pixels: shades between background
// (#ffffff) and series color (#1f77b4). If AA were off, the edge
// pixels would be either pure background or pure series color.
$im = imagecreatetruecolor(800, 600);
(new FastChart\LineChart())
    ->setSize(800, 600)
    ->setSeries([5, 95, 5, 95, 5, 95, 5])
    ->draw($im);

$series0 = (0x1f << 16) | (0x77 << 8) | 0xb4;
$bg = 0xffffff;

$blended = 0;
for ($y = 80; $y < 520; $y++) {
    for ($x = 80; $x < 720; $x++) {
        $c = imagecolorat($im, $x, $y);
        if ($c === $series0) continue;
        if ($c === $bg) continue;

        $r = ($c >> 16) & 0xFF;
        $g = ($c >>  8) & 0xFF;
        $b = ($c)       & 0xFF;
        // A blended pixel between #1f77b4 and white satisfies these
        // bounds: r in [0x1f, 0xfe], g in [0x77, 0xfe], b in [0xb4, 0xfe].
        if ($r > 0x1f && $r < 0xff && $g > 0x77 && $g < 0xff && $b > 0xb4 && $b < 0xff) {
            $blended++;
            if ($blended > 100) break 2;
        }
    }
}
echo "aa_blended_pixels>100: ", ($blended > 100 ? "yes" : "no ($blended)"), "\n";
?>
--EXPECT--
aa_blended_pixels>100: yes
