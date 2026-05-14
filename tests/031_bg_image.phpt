--TEST--
setBackgroundImage composites a source image as canvas background
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

// Build a synthetic "background image" -- a 100x100 PNG filled
// with a unique color we can fingerprint after compositing.
$bg = imagecreatetruecolor(100, 100);
$pink = imagecolorallocate($bg, 0xFF, 0x00, 0xCC);
imagefilledrectangle($bg, 0, 0, 99, 99, $pink);
$bg_path = sys_get_temp_dir() . '/fc_bg_test.png';
imagepng($bg, $bg_path);

$bytes = (new FastChart\LineChart(400, 300))
    ->setBackgroundImage($bg_path)
    ->setSeries([1, 5, 3, 8])
    ->renderPng();
unlink($bg_path);

$im = imagecreatefromstring($bytes);
// Sample a corner -- the bg image fills the whole canvas, so the
// corner should be the magenta-ish bg-image color (or close to it
// after PNG roundtrip).
$rgba = imagecolorat($im, 5, 5);
$r = ($rgba >> 16) & 0xFF;
$g = ($rgba >>  8) & 0xFF;
$b = ($rgba)       & 0xFF;
$is_pinkish = $r > 200 && $g < 50 && $b > 150;
echo "corner_is_bg: ", ($is_pinkish ? "yes" : sprintf("no (#%02x%02x%02x)", $r, $g, $b)), "\n";

// Plot area still readable -- it's drawn on top of the bg image.
ob_start(); imagepng($im); $rerendered = ob_get_clean();
echo "renders_ok: ", strlen($rerendered) > 1024 ? "yes" : "no", "\n";

// Embedded NUL rejected.
try {
    (new FastChart\LineChart)->setBackgroundImage("/etc\0passwd");
    echo "nul_in_path: no throw\n";
} catch (\ValueError $e) {
    echo "nul_in_path: ValueError ok\n";
}

// Empty string clears.
$out = (new FastChart\LineChart)->setBackgroundImage('/tmp/foo.png')->setBackgroundImage('');
var_dump($out instanceof FastChart\LineChart);

// Missing file silently no-ops the bg (chart still renders).
$bytes = (new FastChart\LineChart(400, 300))
    ->setBackgroundImage('/nonexistent/path.png')
    ->setSeries([1, 2, 3])
    ->renderPng();
echo "missing_file_renders: ", strlen($bytes) > 1024 ? "ok" : "bad", "\n";
?>
--EXPECT--
corner_is_bg: yes
renders_ok: yes
nul_in_path: ValueError ok
bool(true)
missing_file_renders: ok
