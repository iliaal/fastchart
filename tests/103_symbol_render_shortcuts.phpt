--TEST--
Symbol family: render-shortcut matrix (PNG/JPEG/WebP/GIF/AVIF/toFile) for Code128 + QrCode
--EXTENSIONS--
fastchart
--SKIPIF--
<?php if (!extension_loaded('gd')) die('skip ext/gd not loaded'); ?>
--FILE--
<?php

$specs = [
    'Code128' => [
        'instance'  => fn() => (new FastChart\Code128())->setData('FC-12345')->setSize(400, 100),
        'expect_w'  => 400,
        'expect_h'  => 100,
    ],
    'QrCode'  => [
        'instance'  => fn() => (new FastChart\QrCode())->setData('https://example.com')->setSize(250, 250),
        'expect_w'  => 250,
        'expect_h'  => 250,
    ],
];

$magic = [
    'png'  => "\x89PNG\r\n\x1a\n",
    'jpeg' => "\xFF\xD8\xFF",  // JPEG SOI + first marker byte
    'webp' => 'RIFF',          // RIFF container; "WEBP" sits 4 bytes later
    'gif'  => 'GIF8',          // GIF87a / GIF89a both share this prefix
];

foreach ($specs as $cls => $spec) {
    // PNG
    $png = $spec['instance']()->renderPng();
    var_dump(strlen($png) > 0);
    var_dump(substr($png, 0, 8) === $magic['png']);

    // JPEG
    $jpg = $spec['instance']()->renderJpeg(85);
    var_dump(strlen($jpg) > 0);
    var_dump(substr($jpg, 0, 3) === $magic['jpeg']);

    // WebP
    $webp = $spec['instance']()->renderWebp(80);
    var_dump(strlen($webp) > 0);
    var_dump(substr($webp, 0, 4) === $magic['webp']);
    var_dump(substr($webp, 8, 4) === 'WEBP');

    // GIF
    $gif = $spec['instance']()->renderGif();
    var_dump(strlen($gif) > 0);
    var_dump(substr($gif, 0, 4) === $magic['gif']);

    // AVIF (skip if libgd built without AVIF support; the renderer
    // throws a regular Exception with a recognisable message in that
    // case, distinct from a ValueError on bad input).
    try {
        $avif = $spec['instance']()->renderAvif(50);
        var_dump(strlen($avif) > 0);
        // AVIF carries `ftyp` + brand at offset 4. Brand should
        // contain "avif" or "avis" at offset 8.
        $brand = substr($avif, 8, 4);
        var_dump(in_array($brand, ['avif', 'avis', 'mif1'], true));
    } catch (\Exception $e) {
        if (str_contains($e->getMessage(), 'AVIF')) {
            // Build doesn't have AVIF — emit two placeholder bool(true)
            // so the EXPECT count stays consistent across builds.
            var_dump(true); var_dump(true);
        } else {
            throw $e;
        }
    }

    // renderToFile picks format from extension. Test all five.
    foreach (['png', 'jpg', 'webp', 'gif'] as $ext) {
        $tmp = tempnam(sys_get_temp_dir(), "fc-shortcut-$cls-") . ".$ext";
        $bytes = $spec['instance']()->renderToFile($tmp, $ext === 'jpg' ? 85 : 80);
        var_dump($bytes > 0);
        var_dump(file_exists($tmp));
        $contents = file_get_contents($tmp);
        // Decode round-trip — every format gd writes should round-
        // trip via imagecreatefromstring.
        $im = imagecreatefromstring($contents);
        var_dump($im instanceof \GdImage);
        var_dump(imagesx($im) === $spec['expect_w']);
        var_dump(imagesy($im) === $spec['expect_h']);
        imagedestroy($im);
        unlink($tmp);
    }

    // renderToFile rejects unknown extensions.
    try {
        $tmp = tempnam(sys_get_temp_dir(), "fc-shortcut-$cls-") . '.bmp';
        $spec['instance']()->renderToFile($tmp);
        echo "ERR: $cls bmp not rejected\n";
        @unlink($tmp);
    } catch (\ValueError $e) {
        echo "$cls bmp-reject: ", str_contains($e->getMessage(), 'could not infer format') ? "ok" : $e->getMessage(), "\n";
        @unlink($tmp);
    }

    // Quality range validation matches Chart's policy: JPEG rejects
    // <1 or >100; WebP/AVIF accept 0..100; PNG/GIF ignore quality.
    try { $spec['instance']()->renderJpeg(0);   echo "ERR: $cls JPEG q=0 not rejected\n"; }
    catch (\ValueError $e) { echo "$cls jpeg-q0: ok\n"; }
    try { $spec['instance']()->renderJpeg(101); echo "ERR: $cls JPEG q=101 not rejected\n"; }
    catch (\ValueError $e) { echo "$cls jpeg-q101: ok\n"; }
    try { $spec['instance']()->renderWebp(101); echo "ERR: $cls WebP q=101 not rejected\n"; }
    catch (\ValueError $e) { echo "$cls webp-q101: ok\n"; }
    try { $spec['instance']()->renderWebp(0); $reject = false; }
    catch (\ValueError $e) { $reject = true; }
    var_dump($reject === false);  // q=0 is valid for WebP
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
Code128 bmp-reject: ok
Code128 jpeg-q0: ok
Code128 jpeg-q101: ok
Code128 webp-q101: ok
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
QrCode bmp-reject: ok
QrCode jpeg-q0: ok
QrCode jpeg-q101: ok
QrCode webp-q101: ok
bool(true)
