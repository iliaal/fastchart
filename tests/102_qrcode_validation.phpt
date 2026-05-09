--TEST--
QrCode: setter + render-time validation
--EXTENSIONS--
fastchart
--FILE--
<?php

// renderPng without setData throws Error (not ValueError) — matches
// the Code128 + Chart family pattern.
try {
    (new FastChart\QrCode())->renderPng();
    echo "ERR: render without setData did not throw\n";
} catch (\Error $e) {
    echo "no-data: ", str_contains($e->getMessage(), 'setData') ? "ok" : $e->getMessage(), "\n";
}

// setEcc rejects out-of-range levels at setter time.
foreach ([-1, 4, 99] as $bad_ecc) {
    try {
        (new FastChart\QrCode())->setEcc($bad_ecc);
        echo "ERR: setEcc($bad_ecc) did not throw\n";
    } catch (\ValueError $e) {
        echo "ecc=$bad_ecc: ok\n";
    }
}

// setMinVersion / setMaxVersion reject out-of-range at setter time.
foreach ([0, 41, -5] as $bad_v) {
    try { (new FastChart\QrCode())->setMinVersion($bad_v); echo "ERR: setMinVersion($bad_v) did not throw\n"; }
    catch (\ValueError $e) { echo "min=$bad_v: ok\n"; }
    try { (new FastChart\QrCode())->setMaxVersion($bad_v); echo "ERR: setMaxVersion($bad_v) did not throw\n"; }
    catch (\ValueError $e) { echo "max=$bad_v: ok\n"; }
}

// Cross-bound check fires at render time, not at setter time. The
// setters intentionally allow transient out-of-order state so that
// `setMinVersion(40)->setMaxVersion(40)` is valid (the intermediate
// state has min=40, max=default 40 — fine).
try {
    (new FastChart\QrCode())
        ->setData('x')
        ->setMinVersion(40)
        ->setMaxVersion(1)
        ->renderPng();
    echo "ERR: range inversion did not throw\n";
} catch (\ValueError $e) {
    echo "range-inverted: ", str_contains($e->getMessage(), 'version range must be non-empty') ? "ok" : $e->getMessage(), "\n";
}

// Encoder cannot fit data within the requested version+ECC range.
// Version 1 (21x21) at ECC L holds at most 17 alphanumeric chars or
// 25 numeric. 30 alphanumeric chars cannot fit.
try {
    (new FastChart\QrCode())
        ->setData(str_repeat('A', 30))
        ->setMaxVersion(1)
        ->setEcc(FastChart\QrCode::ECC_L)
        ->renderPng();
    echo "ERR: too-large payload did not throw\n";
} catch (\ValueError $e) {
    echo "too-large: ", str_contains($e->getMessage(), 'does not fit') ? "ok" : $e->getMessage(), "\n";
}

// Canvas too small for the encoded symbol + quiet zone.
try {
    (new FastChart\QrCode())
        ->setData('hello')
        ->setSize(20, 20)
        ->renderPng();
    echo "ERR: tiny canvas did not throw\n";
} catch (\ValueError $e) {
    echo "tiny-canvas: ", str_contains($e->getMessage(), 'too small') ? "ok" : $e->getMessage(), "\n";
}

// Empty data already rejected at setData (Symbol-level).
try {
    (new FastChart\QrCode())->setData('');
    echo "ERR: empty data did not throw\n";
} catch (\ValueError $e) {
    echo "empty-data: ok\n";
}

// QR-specific quiet zone cap: setQuietZone accepts up to 4096 at the
// Symbol-level setter, but values >256 modules don't make sense for
// QR. Render-time check throws ValueError instead of silently
// clamping, so a caller who sets 4096 sees the rejection.
try {
    (new FastChart\QrCode())
        ->setData('hello')
        ->setSize(2000, 2000)
        ->setQuietZone(300)
        ->renderPng();
    echo "ERR: excessive quiet zone did not throw\n";
} catch (\ValueError $e) {
    echo "qz-cap: ", str_contains($e->getMessage(), 'maximum of 256 modules') ? "ok" : $e->getMessage(), "\n";
}

// Embedded NUL rejected at setData (Symbol-level). Both QR and
// Code128 share this: gdImageStringFT truncates at NUL and the QR
// encoder takes a NUL-terminated string from qrcodegen_encodeText.
try {
    (new FastChart\QrCode())->setData("a\0b");
    echo "ERR: NUL in data did not throw\n";
} catch (\ValueError $e) {
    echo "nul-data: ok\n";
}

?>
--EXPECT--
no-data: ok
ecc=-1: ok
ecc=4: ok
ecc=99: ok
min=0: ok
max=0: ok
min=41: ok
max=41: ok
min=-5: ok
max=-5: ok
range-inverted: ok
too-large: ok
tiny-canvas: ok
empty-data: ok
qz-cap: ok
nul-data: ok
