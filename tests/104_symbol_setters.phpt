--TEST--
Symbol family: setter range validation + fluent chaining
--EXTENSIONS--
fastchart
--FILE--
<?php

// Helper: assert that a closure throws ValueError. Returns true if it
// did, false otherwise. Centralises the try/catch boilerplate.
$rejects = function (callable $f): bool {
    try { $f(); return false; }
    catch (\ValueError $e) { return true; }
};

// All concrete Symbol subclasses share the Symbol-base setters, so
// run the matrix against both Code128 and QrCode. The setters live on
// Symbol; the cast at method entry is the same regardless of leaf.
foreach (['Code128', 'QrCode'] as $cls) {
    $fqcn = "FastChart\\$cls";

    // setSize: width and height must be in [1, 65535].
    var_dump($rejects(fn() => (new $fqcn())->setSize(0, 100)));
    var_dump($rejects(fn() => (new $fqcn())->setSize(100, 0)));
    var_dump($rejects(fn() => (new $fqcn())->setSize(-1, 100)));
    var_dump($rejects(fn() => (new $fqcn())->setSize(70000, 100)));
    var_dump($rejects(fn() => (new $fqcn())->setSize(100, 70000)));

    // setData: empty and embedded NUL rejected at setter time.
    var_dump($rejects(fn() => (new $fqcn())->setData('')));
    var_dump($rejects(fn() => (new $fqcn())->setData("a\0b")));

    // setQuietZone: -1 sentinel ok; -2+ rejected; 4097+ rejected.
    var_dump($rejects(fn() => (new $fqcn())->setQuietZone(-2)));
    var_dump($rejects(fn() => (new $fqcn())->setQuietZone(4097)));

    // setForeground / setBackground: 24-bit RGB only.
    var_dump($rejects(fn() => (new $fqcn())->setForeground(-1)));
    var_dump($rejects(fn() => (new $fqcn())->setForeground(0x1000000)));
    var_dump($rejects(fn() => (new $fqcn())->setBackground(-1)));
    var_dump($rejects(fn() => (new $fqcn())->setBackground(0x1000000)));

    // setDpi: 24..1200.
    var_dump($rejects(fn() => (new $fqcn())->setDpi(23)));
    var_dump($rejects(fn() => (new $fqcn())->setDpi(1201)));
}

// QrCode-specific setters.
var_dump($rejects(fn() => (new FastChart\QrCode())->setEcc(-1)));
var_dump($rejects(fn() => (new FastChart\QrCode())->setEcc(4)));
var_dump($rejects(fn() => (new FastChart\QrCode())->setEcc(99)));
var_dump($rejects(fn() => (new FastChart\QrCode())->setMinVersion(0)));
var_dump($rejects(fn() => (new FastChart\QrCode())->setMinVersion(41)));
var_dump($rejects(fn() => (new FastChart\QrCode())->setMaxVersion(0)));
var_dump($rejects(fn() => (new FastChart\QrCode())->setMaxVersion(41)));

// Boundary values that should be ACCEPTED (no throw).
$acc = function (callable $f): bool {
    try { $f(); return true; }
    catch (\Throwable $e) { return false; }
};
var_dump($acc(fn() => (new FastChart\Code128())->setSize(1, 1)));
var_dump($acc(fn() => (new FastChart\Code128())->setSize(65535, 65535)));
var_dump($acc(fn() => (new FastChart\Code128())->setQuietZone(-1)));    // sentinel
var_dump($acc(fn() => (new FastChart\Code128())->setQuietZone(0)));
var_dump($acc(fn() => (new FastChart\Code128())->setQuietZone(4096)));
var_dump($acc(fn() => (new FastChart\Code128())->setForeground(0)));
var_dump($acc(fn() => (new FastChart\Code128())->setForeground(0xFFFFFF)));
var_dump($acc(fn() => (new FastChart\Code128())->setDpi(24)));
var_dump($acc(fn() => (new FastChart\Code128())->setDpi(1200)));
var_dump($acc(fn() => (new FastChart\QrCode())->setEcc(0)));
var_dump($acc(fn() => (new FastChart\QrCode())->setEcc(3)));
var_dump($acc(fn() => (new FastChart\QrCode())->setMinVersion(1)));
var_dump($acc(fn() => (new FastChart\QrCode())->setMinVersion(40)));
var_dump($acc(fn() => (new FastChart\QrCode())->setMaxVersion(1)));
var_dump($acc(fn() => (new FastChart\QrCode())->setMaxVersion(40)));

// Fluent chaining: every setter returns $this. A long chain ending in
// a render must produce non-empty output; the chained instance must
// be the same object as the leaf return.
$obj = new FastChart\Code128();
$ret = $obj
    ->setData('CHAIN')
    ->setSize(300, 80)
    ->setForeground(0x222222)
    ->setBackground(0xFAFAFA)
    ->setQuietZone(8)
    ->setShowText(false)
    ->setDpi(150)
    ->setTransparentBackground(false);
var_dump($ret === $obj);
var_dump(strlen($ret->renderPng()) > 0);

$qr = new FastChart\QrCode();
$ret = $qr
    ->setData('QR-CHAIN')
    ->setSize(250, 250)
    ->setEcc(FastChart\QrCode::ECC_Q)
    ->setMinVersion(1)
    ->setMaxVersion(10)
    ->setQuietZone(4)
    ->setForeground(0x000000)
    ->setBackground(0xFFFFFF)
    ->setDpi(96)
    ->setTransparentBackground(true);
var_dump($ret === $qr);
var_dump(strlen($ret->renderPng()) > 0);

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
