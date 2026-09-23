<?php
/* Shared font and DPI defaults for the examples. Picks a TrueType
 * face from common system paths; override with the FC_FONT
 * environment variable. Each example does:
 *
 *     require __DIR__ . '/_bootstrap.php';
 *     // ... ->setFontPath($font)->setDpi($dpi) ...
 *
 * fastchart's MINIT also probes these paths for a default font; the
 * explicit setFontPath call keeps the examples portable.
 *
 * DPI defaults to 200, so examples render at about 2x the logical
 * canvas size (setSize(640, 320) -> 1333x667 pixels) with 200 DPI PNG
 * metadata. Set FC_DPI for 96 (1x) or 300 (print). */

$font_candidates = [
    getenv('FC_FONT') ?: '',
    /* Lato preferred: UI-grade sans, hints cleanly at axis-label sizes. */
    '/usr/share/fonts/truetype/lato/Lato-Medium.ttf',            // Debian / Ubuntu (fonts-lato)
    '/usr/share/fonts/truetype/lato/Lato-Regular.ttf',
    '/usr/share/fonts/lato/Lato-Medium.ttf',                     // Fedora / RHEL
    '/usr/share/fonts/lato/Lato-Regular.ttf',
    '/usr/share/fonts/TTF/Lato-Medium.ttf',                      // Arch
    '/usr/share/fonts/TTF/Lato-Regular.ttf',
    /* DejaVu fallback: present on most Linux base installs. */
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
