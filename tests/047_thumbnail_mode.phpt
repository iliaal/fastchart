--TEST--
setThumbnailMode shrinks fonts and elides labels for tiny renders
--EXTENSIONS--
fastchart
gd
--FILE--
<?php

// Sparkline-style thumbnail, no labels.
$bytes = (new FastChart\LineChart(120, 60))
    ->setThumbnailMode(true)
    ->setTitle('Hidden')
    ->setSeries([1, 5, 3, 8, 4, 7, 5])
    ->renderPng();
$im = imagecreatefromstring($bytes);

// Default text color is #222222 in light theme. With thumbnail,
// title is suppressed so very few text pixels should remain.
$text_pixels = 0;
for ($y = 0; $y < 60; $y++)
    for ($x = 0; $x < 120; $x++)
        if (imagecolorat($im, $x, $y) === 0x222222) $text_pixels++;
echo "thumbnail_few_text: ", ($text_pixels < 20 ? "yes" : "no ($text_pixels)"), "\n";

// Reference render at the same size with thumbnail off should
// have noticeably more text pixels (title + labels).
$bytes_ref = (new FastChart\LineChart(120, 60))
    ->setTitle('Hidden')
    ->setSeries([1, 5, 3, 8, 4, 7, 5])
    ->renderPng();
$im_ref = imagecreatefromstring($bytes_ref);
$ref_text = 0;
for ($y = 0; $y < 60; $y++)
    for ($x = 0; $x < 120; $x++)
        if (imagecolorat($im_ref, $x, $y) === 0x222222) $ref_text++;
echo "ref_more_text: ", ($ref_text > $text_pixels ? "yes" : "no"), "\n";
?>
--EXPECT--
thumbnail_few_text: yes
ref_more_text: yes
