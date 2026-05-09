<?php
/* QR Code variants. Same payload, all four error-correction levels.
 *
 * The encoder picks the smallest QR version that fits the data at
 * the requested ECC level — higher ECC eats codeword space and may
 * push the symbol up a version (more modules per side, denser look),
 * but the decoded payload survives more pixel damage:
 *   - ECC_L: ~7%  recovery  (smallest symbol)
 *   - ECC_M: ~15% recovery  (default; balanced)
 *   - ECC_Q: ~25% recovery  (logo overlays, partial occlusion)
 *   - ECC_H: ~30% recovery  (industrial / outdoor / artistic QR)
 *
 * Use ECC_H when the symbol may be physically damaged or printed
 * over a textured background; use ECC_L for QR codes embedded in
 * digital pipelines where the image survives intact.
 */

require __DIR__ . '/_bootstrap.php';

$payload = 'https://github.com/iliaal/fastchart';

$levels = [
    ['L', FastChart\QrCode::ECC_L, '42a_qrcode_ecc_l.png'],
    ['M', FastChart\QrCode::ECC_M, '42b_qrcode_ecc_m.png'],
    ['Q', FastChart\QrCode::ECC_Q, '42c_qrcode_ecc_q.png'],
    ['H', FastChart\QrCode::ECC_H, '42d_qrcode_ecc_h.png'],
];

foreach ($levels as [$label, $ecc, $file]) {
    (new FastChart\QrCode())
        ->setData($payload)
        ->setSize(300, 300)
        ->setDpi($dpi)
        ->setEcc($ecc)
        ->setQuietZone(4)
        ->setForeground(0x000000)
        ->setBackground(0xFFFFFF)
        ->renderToFile(__DIR__ . '/' . $file);
}
