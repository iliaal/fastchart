--TEST--
Symbol family: render-shortcut matrix (PNG/JPEG/WebP/toFile) for Code128 + QrCode
--EXTENSIONS--
fastchart
gd
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

    // GIF and AVIF were dropped in v1.0 — see test 025.
    try { $spec['instance']()->renderGif();  echo "ERR\n"; }
    catch (\Error $e) { echo "$cls gif-dropped: ok\n"; }
    try { $spec['instance']()->renderAvif(); echo "ERR\n"; }
    catch (\Error $e) { echo "$cls avif-dropped: ok\n"; }

    // renderToFile picks format from extension. Test the three surviving formats.
    foreach (['png', 'jpg', 'webp'] as $ext) {
        $tmp = tempnam(sys_get_temp_dir(), "fc-shortcut-$cls-") . ".$ext";
        $bytes = $spec['instance']()->renderToFile($tmp, $ext === 'jpg' ? 85 : 80);
        var_dump($bytes > 0);
        var_dump(file_exists($tmp));
        $contents = file_get_contents($tmp);
        $im = imagecreatefromstring($contents);
        var_dump($im instanceof \GdImage);
        var_dump(imagesx($im) === $spec['expect_w']);
        var_dump(imagesy($im) === $spec['expect_h']);
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

    // Quality range: JPEG rejects <1 or >100; WebP same since v1.0.
    try { $spec['instance']()->renderJpeg(0);   echo "ERR: $cls JPEG q=0\n"; }
    catch (\ValueError $e) { echo "$cls jpeg-q0: ok\n"; }
    try { $spec['instance']()->renderJpeg(101); echo "ERR: $cls JPEG q=101\n"; }
    catch (\ValueError $e) { echo "$cls jpeg-q101: ok\n"; }
    try { $spec['instance']()->renderWebp(101); echo "ERR: $cls WebP q=101\n"; }
    catch (\ValueError $e) { echo "$cls webp-q101: ok\n"; }
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
Code128 gif-dropped: ok
Code128 avif-dropped: ok
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
QrCode gif-dropped: ok
QrCode avif-dropped: ok
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