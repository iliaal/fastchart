<?php
/* Shared font + DPI defaults for the example gallery. Picks a
 * TrueType face from common Linux paths; override at runtime with
 * the FC_FONT environment variable. Each example does:
 *
 *     require __DIR__ . '/_bootstrap.php';
 *     // ... ->setFontPath($font)->setDpi($dpi) ...
 *
 * fastchart's MINIT also probes the font paths for an implicit
 * default, so the explicit setFontPath call is mainly belt-and-
 * suspenders for portability and self-documentation.
 *
 * The DPI default is 200 — examples render at roughly 2x the logical
 * canvas dimensions (e.g. setSize(640, 320) -> 1333x667 pixels) and
 * tag the PNG metadata as 200 DPI. Retina viewers and print pipelines
 * display these at the intended physical size; embedding into a
 * non-DPI-aware viewer just shows a higher-resolution image. Override
 * via the FC_DPI env var if you want 96 (1x) or higher (300 print). */

$font_candidates = [
    getenv('FC_FONT') ?: '',
    /* Lato preferred — UI-grade sans, hints cleanly at axis-label sizes. */
    '/usr/share/fonts/truetype/lato/Lato-Medium.ttf',            // Debian / Ubuntu (fonts-lato)
    '/usr/share/fonts/truetype/lato/Lato-Regular.ttf',
    '/usr/share/fonts/lato/Lato-Medium.ttf',                     // Fedora / RHEL
    '/usr/share/fonts/lato/Lato-Regular.ttf',
    '/usr/share/fonts/TTF/Lato-Medium.ttf',                      // Arch
    '/usr/share/fonts/TTF/Lato-Regular.ttf',
    /* DejaVu fallback — present on most Linux base installs. */
    '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',
    '/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf',
    '/usr/share/fonts/TTF/DejaVuSans.ttf',
    '/usr/share/fonts/dejavu/DejaVuSans.ttf',
    /* macOS fallbacks. */
    '/Library/Fonts/Arial.ttf',
    '/System/Library/Fonts/Helvetica.ttc',
];
$font = '';
foreach ($font_candidates as $candidate) {
    if ($candidate !== '' && is_file($candidate)) {
        $font = $candidate;
        break;
    }
}
if ($font === '') {
    fwrite(STDERR, "no TrueType font found; set FC_FONT=/path/to/Font.ttf\n");
    exit(1);
}

$dpi = (int)(getenv('FC_DPI') ?: 200);
if ($dpi < 24 || $dpi > 1200) {
    fwrite(STDERR, "FC_DPI must be in [24, 1200]; got $dpi\n");
    exit(1);
}
