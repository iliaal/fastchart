--TEST--
LineChart::draw renders into a GdImage and reaches the plot area
--EXTENSIONS--
fastchart
--FILE--
<?php

$im = imagecreatetruecolor(800, 600);

$out = (new FastChart\LineChart())
    ->setSize(800, 600)
    ->setTitle('Line test')
    ->setTheme(FastChart\Chart::THEME_LIGHT)
    ->setSeries([10, 20, 30, 40, 30, 20, 10])
    ->draw($im);

var_dump($out instanceof \GdImage);
var_dump($out === $im);

// Light theme: canvas background is white (#ffffff).
$bg = imagecolorat($im, 5, 5);
printf("bg=#%06x\n", $bg);

// Series 0 in light theme is RGB 31,119,180 = #1f77b4. Scan a band
// inside the plot area (x: 100-700, y: 100-500) for that color.
$series0 = (0x1f << 16) | (0x77 << 8) | 0xb4;
$hits = 0;
for ($y = 100; $y < 500; $y += 4) {
    for ($x = 100; $x < 700; $x += 4) {
        if (imagecolorat($im, $x, $y) === $series0) {
            $hits++;
        }
    }
}
echo "series0_hits>10: ", ($hits > 10 ? "yes" : "no ($hits)"), "\n";

// Encode to PNG and verify a real PNG header lands in the buffer.
ob_start();
imagepng($im);
$png = ob_get_clean();
echo "png_sig: ", substr(bin2hex($png), 0, 8) === '89504e47' ? "ok" : "bad", "\n";
echo "png_size>1k: ", (strlen($png) > 1024 ? "yes" : "no"), "\n";
?>
--EXPECT--
bool(true)
bool(true)
bg=#ffffff
series0_hits>10: yes
png_sig: ok
png_size>1k: yes
