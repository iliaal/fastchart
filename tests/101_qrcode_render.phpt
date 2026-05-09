--TEST--
QrCode: encoder + renderer produce a structurally valid symbol grid
--EXTENSIONS--
fastchart
--FILE--
<?php

// Render a deterministic payload at a known canvas size. "hello"
// fits in QR version 1 (21x21 modules) at any ECC level; with the
// default 4-module quiet zone, total = 21+8 = 29 modules per side.
// At canvas 290x290, module_px = 290/29 = 10 pixels.
$png = (new FastChart\QrCode())
    ->setData('hello')
    ->setSize(290, 290)
    ->setEcc(FastChart\QrCode::ECC_M)
    ->renderPng();

var_dump(strlen($png) > 0);
var_dump(substr($png, 0, 8) === "\x89PNG\r\n\x1a\n");

$im = imagecreatefromstring($png);
$W = imagesx($im); $H = imagesy($im);
var_dump($W === 290 && $H === 290);

// Sample dark pixel density. Real QR symbols carry several hundred
// dark pixels even at the smallest version; a blank fill is 0.
$dark = 0;
for ($y = 0; $y < $H; $y += 4) {
    for ($x = 0; $x < $W; $x += 4) {
        if ((imagecolorat($im, $x, $y) & 0xFFFFFF) < 0x808080) $dark++;
    }
}
var_dump($dark > 100);

// Position-detection patterns. Each finder is a 7x7 outline with a
// 5x5 inner light ring and a 3x3 dark centre block. At module_px=10
// with quiet=4 modules, symbol module (M, N) maps to canvas pixel
// (40+M*10, 40+N*10).
//
//   - Top-left finder spans modules (0..6, 0..6); centre at (3, 3) →
//     pixel (75, 75): DARK.
//   - White gap of top-left finder is the 5x5 ring at modules
//     (1..5, 1..5) excluding the inner 3x3 dark block. Module (1, 1)
//     is in that ring → pixel (55, 55): LIGHT.
//   - Top-right finder spans modules (14..20, 0..6); centre (17, 3)
//     → pixel (215, 75): DARK.
//   - Bottom-left finder centre at module (3, 17) → pixel (75, 215):
//     DARK.
$sample = function ($x, $y) use ($im) {
    return (imagecolorat($im, $x, $y) & 0xFFFFFF) < 0x808080;
};
var_dump($sample(75, 75));    // top-left finder centre — dark
var_dump(!$sample(55, 55));   // top-left finder white ring — light
var_dump($sample(215, 75));   // top-right finder centre — dark
var_dump($sample(75, 215));   // bottom-left finder centre — dark

// All four ECC levels encode and render.
foreach ([
    FastChart\QrCode::ECC_L,
    FastChart\QrCode::ECC_M,
    FastChart\QrCode::ECC_Q,
    FastChart\QrCode::ECC_H,
] as $ecc) {
    $bytes = (new FastChart\QrCode())
        ->setData('test')
        ->setSize(200, 200)
        ->setEcc($ecc)
        ->renderPng();
    var_dump(strlen($bytes) > 0);
}

// JPEG / WebP / AVIF round-trip.
$qr = (new FastChart\QrCode())->setData('JPEG check')->setSize(200, 200);
var_dump(strlen($qr->renderJpeg(80)) > 0);
var_dump(strlen($qr->renderWebp(80)) > 0);

// renderToFile picks format from extension.
$tmp = tempnam(sys_get_temp_dir(), 'fc-qr-') . '.png';
$bytes = (new FastChart\QrCode())
    ->setData('https://example.com')
    ->setSize(250, 250)
    ->renderToFile($tmp);
var_dump($bytes > 0);
$decoded = imagecreatefromstring(file_get_contents($tmp));
var_dump(imagesx($decoded) === 250 && imagesy($decoded) === 250);
unlink($tmp);

// Larger payload requires a higher version automatically. Encoder
// picks the smallest version that fits within [min_version,
// max_version]; default is [1, 40]. A 100-char alphanumeric payload
// needs ~version 5+.
$bytes = (new FastChart\QrCode())
    ->setData(str_repeat('ABC123 ', 14))
    ->setSize(400, 400)
    ->renderPng();
var_dump(strlen($bytes) > 100);

// Custom quiet zone (in modules, per the QrCode setter contract).
$bytes = (new FastChart\QrCode())
    ->setData('quiet test')
    ->setSize(300, 300)
    ->setQuietZone(8)
    ->renderPng();
var_dump(strlen($bytes) > 0);

// Custom fg/bg colours.
$bytes = (new FastChart\QrCode())
    ->setData('colour')
    ->setSize(200, 200)
    ->setForeground(0x002244)
    ->setBackground(0xFFEECC)
    ->renderPng();
var_dump(strlen($bytes) > 0);

// DPI metadata flows through (Symbol::setDpi → gdImageSetResolution).
$png = (new FastChart\QrCode())
    ->setData('dpi')
    ->setSize(150, 150)
    ->setDpi(300)
    ->renderPng();
$im = imagecreatefromstring($png);
$res = imageresolution($im);
var_dump($res === [300, 300]);

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
bool(true)
bool(true)
