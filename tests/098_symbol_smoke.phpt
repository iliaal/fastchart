--TEST--
Symbol family smoke: hierarchy registers, abstracts reject, renderPng round-trips
--EXTENSIONS--
fastchart
--FILE--
<?php

var_dump(class_exists('FastChart\\Symbol'));
var_dump(class_exists('FastChart\\Barcode'));
var_dump(class_exists('FastChart\\Code128'));
var_dump(class_exists('FastChart\\QrCode'));

// Symbol and Barcode are abstract.
foreach (['Symbol', 'Barcode'] as $cls) {
    try {
        $fqcn = "FastChart\\$cls";
        new $fqcn();
        echo "ERR: $cls instantiated\n";
    } catch (\Error $e) {
        echo "$cls abstract: ", $e->getMessage(), "\n";
    }
}

// Code128 and QrCode are concrete and inherit from Symbol.
var_dump((new FastChart\Code128()) instanceof FastChart\Symbol);
var_dump((new FastChart\Code128()) instanceof FastChart\Barcode);
var_dump((new FastChart\QrCode()) instanceof FastChart\Symbol);

// QrCode ECC class constants exist with expected values.
var_dump(FastChart\QrCode::ECC_L);
var_dump(FastChart\QrCode::ECC_M);
var_dump(FastChart\QrCode::ECC_Q);
var_dump(FastChart\QrCode::ECC_H);

// Setters chain (fluent).
$code = (new FastChart\Code128())
    ->setData('FASTCHART-12345')
    ->setSize(400, 100)
    ->setForeground(0x000000)
    ->setBackground(0xFFFFFF)
    ->setQuietZone(10)
    ->setShowText(true)
    ->setDpi(96);
var_dump($code instanceof FastChart\Code128);

$qr = (new FastChart\QrCode())
    ->setData('https://example.com/order/12345')
    ->setSize(300, 300)
    ->setEcc(FastChart\QrCode::ECC_M)
    ->setMinVersion(1)
    ->setMaxVersion(40)
    ->setQuietZone(4);
var_dump($qr instanceof FastChart\QrCode);

// renderPng emits a valid PNG.
$png = $code->renderPng();
var_dump(strlen($png) > 0);
var_dump(substr($png, 0, 8) === "\x89PNG\r\n\x1a\n");

$png = $qr->renderPng();
var_dump(strlen($png) > 0);
var_dump(substr($png, 0, 8) === "\x89PNG\r\n\x1a\n");

// renderJpeg / renderWebp each produce non-empty output. GIF and AVIF
// were dropped in v1.0 — see test 025.
var_dump(strlen($code->renderJpeg(80)) > 0);
var_dump(strlen($code->renderWebp(80)) > 0);

// renderToFile honours the extension.
$tmp = tempnam(sys_get_temp_dir(), 'fc-symbol-') . '.png';
$bytes = $qr->renderToFile($tmp);
var_dump($bytes > 0);
var_dump(file_exists($tmp));
$round = file_get_contents($tmp);
var_dump(substr($round, 0, 8) === "\x89PNG\r\n\x1a\n");
unlink($tmp);

// Render without setData throws.
try {
    (new FastChart\QrCode())->renderPng();
    echo "ERR: render-without-data did not throw\n";
} catch (\Error $e) {
    echo "no-data: ok\n";
}

// Userland cannot subclass the abstract bases. ZEND_ACC_ABSTRACT alone
// does NOT block this — `class MySym extends Symbol {}; new MySym()`
// would otherwise allocate a vanilla zend_object that cannot back the
// typed C struct, and any inherited setter would walk OOB. The
// abstract-create_object trampoline catches this.
foreach (['Symbol', 'Barcode'] as $abstract) {
    $code = "namespace UserExt$abstract; class MySym extends \\FastChart\\$abstract {}";
    eval($code);
    $cls = "UserExt$abstract\\MySym";
    try {
        new $cls();
        echo "ERR: userland subclass of $abstract instantiated\n";
    } catch (\Error $e) {
        echo "subclass-$abstract: ", str_contains($e->getMessage(), 'cannot be instantiated or subclassed') ? "ok" : $e->getMessage(), "\n";
    }
}

// transparent_bg must produce real pixel alpha in the encoded PNG,
// not just the gd transparent-color-index flag (which does nothing on
// a truecolor image). We re-decode the PNG and read the alpha channel
// at a known background pixel.
$png = (new FastChart\QrCode())
    ->setData('hello')
    ->setSize(50, 50)
    ->setBackground(0xFFFFFF)
    ->setTransparentBackground(true)
    ->renderPng();
$im = imagecreatefromstring($png);
$bg_pixel = imagecolorat($im, 0, 0);
$alpha = ($bg_pixel >> 24) & 0x7F;
var_dump($alpha === 127);  // 127 = fully transparent in libgd's 0..127 alpha range

// Without transparent_bg, the same pixel is fully opaque (alpha = 0).
$png = (new FastChart\QrCode())
    ->setData('hello')
    ->setSize(50, 50)
    ->setBackground(0xFFFFFF)
    ->renderPng();
$im = imagecreatefromstring($png);
$bg_pixel = imagecolorat($im, 0, 0);
$alpha = ($bg_pixel >> 24) & 0x7F;
var_dump($alpha === 0);

// Validation rejects out-of-range arguments.
$cases = [
    fn() => (new FastChart\Code128())->setSize(0, 100),
    fn() => (new FastChart\Code128())->setSize(70000, 100),
    fn() => (new FastChart\Code128())->setData(''),
    fn() => (new FastChart\Code128())->setData("a\0b"),
    fn() => (new FastChart\Code128())->setQuietZone(-2),
    fn() => (new FastChart\Code128())->setForeground(0x1000000),
    fn() => (new FastChart\Code128())->setBackground(-1),
    fn() => (new FastChart\Code128())->setDpi(10),
    fn() => (new FastChart\Code128())->setDpi(2400),
    fn() => (new FastChart\QrCode())->setEcc(99),
    fn() => (new FastChart\QrCode())->setMinVersion(0),
    fn() => (new FastChart\QrCode())->setMaxVersion(41),
];
$rejected = 0;
foreach ($cases as $f) {
    try { $f(); }
    catch (\ValueError $e) { $rejected++; }
}
var_dump($rejected === count($cases));

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
Symbol abstract: Cannot instantiate abstract class FastChart\Symbol
Barcode abstract: Cannot instantiate abstract class FastChart\Barcode
bool(true)
bool(true)
bool(true)
int(0)
int(1)
int(2)
int(3)
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
no-data: ok
subclass-Symbol: ok
subclass-Barcode: ok
bool(true)
bool(true)
bool(true)
