<?php
/* Pictogram (pictorial fraction): a grid of unit icons where a value's
 * share of a total is shown by filling that fraction left to right; the
 * boundary icon is partially clipped so fractional values read exactly.
 * The "X in 10 people" infographic style. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\Pictogram(560, 280))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('73% of users enabled 2FA')
    ->setTotal(100)
    ->setValue(73)
    ->setIconCount(100)
    ->setColumns(10)
    ->setShape(FastChart\Pictogram::SHAPE_PERSON)
    ->setFillColor(0x2E7D32)
    ->setEmptyColor(0xD7DCE0)
    ->renderToFile(__DIR__ . '/63_pictogram.png');
