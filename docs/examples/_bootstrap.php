<?php
/* Shared font resolution for the example gallery. Picks a TrueType
 * face from common Linux paths; override at runtime with the
 * FC_FONT environment variable. Each example does:
 *
 *     require __DIR__ . '/_bootstrap.php';
 *     // ... ->setFontPath($font) ...
 *
 * fastchart's MINIT also probes these paths for an implicit default,
 * so the explicit setFontPath call is mainly belt-and-suspenders for
 * portability and self-documentation. */

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
