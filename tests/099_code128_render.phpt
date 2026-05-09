--TEST--
Code128: encoder + renderer produce structurally valid bar pattern
--EXTENSIONS--
fastchart
--FILE--
<?php

// Render a small, deterministic payload at a known canvas size.
// Subset switches: start C (digit pair "12") → pair "34" → Code-B
// (odd trailing digit) → '5' in B → mod-103 checksum → stop.
$png = (new FastChart\Code128())
    ->setData('12345')
    ->setSize(200, 80)
    ->setForeground(0x000000)
    ->setBackground(0xFFFFFF)
    ->renderPng();

var_dump(strlen($png) > 0);
var_dump(substr($png, 0, 8) === "\x89PNG\r\n\x1a\n");

$im = imagecreatefromstring($png);
$W = imagesx($im); $H = imagesy($im);
var_dump($W === 200 && $H === 80);

// Scan the middle row. Count bar→space and space→bar transitions.
// "12345" produces ~44 transitions on this canvas; assert >= 30 as a
// loose lower bound — a blank fill produces 0, a structurally
// correct barcode is comfortably above 30.
$y = (int)($H / 2);
$transitions = 0;
$prev_dark = null;
$first_dark_x = -1;
$last_dark_x = -1;
for ($x = 0; $x < $W; $x++) {
    $rgb = imagecolorat($im, $x, $y) & 0xFFFFFF;
    $is_dark = $rgb < 0x808080;
    if ($is_dark) {
        if ($first_dark_x < 0) $first_dark_x = $x;
        $last_dark_x = $x;
    }
    if ($prev_dark !== null && $is_dark !== $prev_dark) $transitions++;
    $prev_dark = $is_dark;
}
imagedestroy($im);

// At least 30 transitions on the mid-row.
var_dump($transitions >= 30);
// First dark pixel sits at the start of the bar region (10-module
// quiet zone = 20px @ 2px/module).
var_dump($first_dark_x >= 18 && $first_dark_x <= 22);
// Last dark pixel sits within ~5px of the right-side bar end. Allow
// some slop because integer module rounding leaves slack pixels in
// the right quiet zone.
var_dump($last_dark_x >= $W - 25 && $last_dark_x < $W - 15);

// Render with show_text. The text strip occupies the bottom slice of
// the canvas; bar pixels should still appear in the upper portion.
$png2 = (new FastChart\Code128())
    ->setData('FASTCHART-12345')
    ->setSize(400, 100)
    ->setShowText(true)
    ->renderPng();
var_dump(substr($png2, 0, 8) === "\x89PNG\r\n\x1a\n");
$im2 = imagecreatefromstring($png2);
// Bars exist in the upper half (above the text strip).
$bar_dark_pixels = 0;
for ($x = 0; $x < imagesx($im2); $x++) {
    $rgb = imagecolorat($im2, $x, 30) & 0xFFFFFF;
    if ($rgb < 0x808080) $bar_dark_pixels++;
}
imagedestroy($im2);
var_dump($bar_dark_pixels > 30);

// Render JPEG / WebP / AVIF too — round-trip through the dispatch.
foreach (['Jpeg' => 90, 'Webp' => 80] as $fmt => $q) {
    $bytes = (new FastChart\Code128())
        ->setData('TEST123')
        ->setSize(300, 80)
        ->{"render$fmt"}($q);
    var_dump(strlen($bytes) > 0);
}

// Odd-digit-tail optimization: "12345" (5 digits, odd, all digits)
// must encode as START_B + '1' + Code-C + "23" + "45" + checksum +
// STOP = 7 codes = 79 modules, NOT START_B + 5 chars + checksum +
// STOP = 8 codes = 90 modules. The shorter encoding lets us render
// at canvas width 99 (= 79 modules + 20 module auto quiet zone @
// module_px=1); the unoptimised encoding would require width ≥ 110.
$bytes = (new FastChart\Code128())->setData('12345')->setSize(99, 50)->renderPng();
var_dump(strlen($bytes) > 0);

// DPI metadata flows through to the encoded PNG. setDpi(200) must
// stamp 200 DPI in the PNG pHYs chunk; libgd defaults to 96 if
// gdImageSetResolution is not called.
$png = (new FastChart\Code128())->setData('ABC')->setSize(200, 80)->setDpi(200)->renderPng();
$im = imagecreatefromstring($png);
$res = imageresolution($im);
imagedestroy($im);
var_dump($res === [200, 200]);

// Subsets cover printable ASCII, control chars (subset A via shift),
// long digit runs (subset C). All produce non-empty PNG output.
$payloads = [
    'Hello, World!',          // pure subset B
    '0123456789',             // pure subset C (10 digits)
    'A1B2C3D4',               // mixed: B with short digit runs
    "Tab\tHere",              // subset B with shift to A for \t
    "9876543210FOO",          // subset C → B switch
    str_repeat('X', 80),      // exactly at the size cap
];
foreach ($payloads as $p) {
    // Pure-B 80-char payloads need ~935px width at module=1; bump the
    // canvas so the largest test payload doesn't run out of pixels.
    $bytes = (new FastChart\Code128())->setData($p)->setSize(1000, 80)->renderPng();
    var_dump(strlen($bytes) > 100);
}

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
