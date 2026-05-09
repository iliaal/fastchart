<?php
/* Code 128 barcode variants. Same encoder, three rendering modes:
 *   - 41a: alphanumeric payload with auto-switching (subset B, then
 *     odd-tail optimisation flips to subset C for the trailing 12345),
 *     human-readable text below.
 *   - 41b: pure-digit payload encoded entirely in subset C (digit
 *     pairs, dense). Shorter modules → smaller canvas suffices.
 *   - 41c: same alphanumeric payload as 41a but with show_text=false,
 *     producing a compact bars-only barcode for embedding in tight
 *     spaces (header strips, ticket stubs, inventory tags).
 *
 * Symbol family is render-only; renderToFile picks the format from
 * the file extension.
 */

require __DIR__ . '/_bootstrap.php';

(new FastChart\Code128())
    ->setData('FASTCHART-12345')
    ->setSize(400, 100)
    ->setDpi($dpi)
    ->setShowText(true)
    ->renderToFile(__DIR__ . '/41a_code128_alphanumeric.png');

(new FastChart\Code128())
    ->setData('0123456789012345')
    ->setSize(360, 80)
    ->setDpi($dpi)
    ->setShowText(true)
    ->renderToFile(__DIR__ . '/41b_code128_numeric.png');

(new FastChart\Code128())
    ->setData('FASTCHART-12345')
    ->setSize(400, 60)
    ->setDpi($dpi)
    ->setShowText(false)
    ->renderToFile(__DIR__ . '/41c_code128_compact.png');
