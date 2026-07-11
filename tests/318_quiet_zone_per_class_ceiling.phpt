--TEST--
setQuietZone enforces a per-class ceiling (QrCode 256 modules, Code128 4096 px)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The QR render path rejects a quiet zone above 256 modules, but the
 * setter used to accept up to 4096 for every symbol class, so
 * setQuietZone(257) succeeded and then every render threw. The setter
 * now validates against a per-class ceiling: modules for QrCode,
 * pixels for Code128. */

$qr = (new FastChart\QrCode())->setData('HELLO');
try {
    $qr->setQuietZone(257);
    echo "qr 257: NO THROW\n";
} catch (\ValueError $e) {
    echo "qr 257: ",
        $e->getMessage() === 'FastChart\QrCode::setQuietZone() quiet zone must be in [-1, 256] modules'
        ? "ok" : $e->getMessage(), "\n";
}

// 256 is the ceiling: accepted and renders (canvas sized for it).
$qr->setSize(600, 600)->setQuietZone(256);
echo "qr 256 renders: ", strlen($qr->renderSvg()) > 0 ? "ok" : "FAIL", "\n";

// -1 sentinel still works.
$qr->setQuietZone(-1);
echo "qr -1 renders: ", strlen($qr->renderSvg()) > 0 ? "ok" : "FAIL", "\n";

// Code128 counts pixels: 300 is well under its 4096 ceiling.
$c128 = (new FastChart\Code128())->setData('ABC')->setSize(1200, 200);
$c128->setQuietZone(300);
echo "code128 300 renders: ", strlen($c128->renderSvg()) > 0 ? "ok" : "FAIL", "\n";
$c128->setQuietZone(-1);
echo "code128 -1 renders: ", strlen($c128->renderSvg()) > 0 ? "ok" : "FAIL", "\n";

// Code128 above its own ceiling is rejected in pixel terms.
try {
    (new FastChart\Code128())->setQuietZone(4097);
    echo "code128 4097: NO THROW\n";
} catch (\ValueError $e) {
    echo "code128 4097: ",
        $e->getMessage() === 'FastChart\Code128::setQuietZone() quiet zone must be in [-1, 4096] pixels'
        ? "ok" : $e->getMessage(), "\n";
}

echo "done\n";
?>
--EXPECT--
qr 257: ok
qr 256 renders: ok
qr -1 renders: ok
code128 300 renders: ok
code128 -1 renders: ok
code128 4097: ok
done
