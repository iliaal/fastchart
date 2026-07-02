--TEST--
Symbol renderWebp rejects libwebp's 16383-pixel per-dimension cap up front
--EXTENSIONS--
fastchart
--FILE--
<?php

try {
    (new FastChart\Code128())
        ->setData('ABC123')
        ->setSize(16384, 80)
        ->renderWebp();
    echo "symbol_webp_cap: NO THROW\n";
} catch (Error $e) {
    echo "symbol_webp_cap: ",
        (str_contains($e->getMessage(), '16383') ? 'throws' : $e->getMessage()), "\n";
}

try {
    (new FastChart\QrCode())
        ->setData('ABC123')
        ->setSize(80, 16384)
        ->renderWebp();
    echo "qrcode_webp_cap: NO THROW\n";
} catch (Error $e) {
    echo "qrcode_webp_cap: ",
        (str_contains($e->getMessage(), '16383') ? 'throws' : $e->getMessage()), "\n";
}

?>
--EXPECT--
symbol_webp_cap: throws
qrcode_webp_cap: throws
