--TEST--
addTextAnnotation paints free-floating labels at canvas coordinates
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

$bytes = (new FastChart\LineChart(500, 400))
    ->setSeries([1, 5, 3, 8])
    ->addTextAnnotation('Note A', 100, 100, 0xFF0000)
    ->addTextAnnotation('Note B', 250, 200, 0x00CC00)
    ->renderPng();
$im = imagecreatefromstring($bytes);

$has_color = function ($im, $color, $w, $h) {
    for ($y = 0; $y < $h; $y++)
        for ($x = 0; $x < $w; $x++)
            if (imagecolorat($im, $x, $y) === $color) return true;
    return false;
};

echo "red_note_present: ", $has_color($im, 0xFF0000, 500, 400) ? "yes" : "no", "\n";
echo "green_note_present: ", $has_color($im, 0x00CC00, 500, 400) ? "yes" : "no", "\n";

// Default color when null/omitted (uses theme text color).
$bytes2 = (new FastChart\LineChart(400, 300))
    ->setSeries([1, 2, 3])
    ->addTextAnnotation('default', 100, 100)
    ->renderPng();
echo "default_color_renders: ", strlen($bytes2) > 1024 ? "ok" : "bad", "\n";

// Out-of-range color rejected.
try {
    (new FastChart\LineChart)->addTextAnnotation('x', 0, 0, 0x1000000);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}
?>
--EXPECT--
red_note_present: yes
green_note_present: yes
default_color_renders: ok
bad: ValueError ok
