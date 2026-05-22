<?php
/* Shared font-candidate list for phpts that need a system TrueType
 * face. Single source of truth — both --SKIPIF-- and --FILE-- blocks
 * pull from here so a new distro / repackage path only needs to be
 * added once. docs/examples/_bootstrap.php has a wider list that
 * also covers macOS; phpts skip on non-Linux so this stays
 * Linux-only.
 *
 * EXTENSION NOTE — this file is `.inc`, not `.inc.php`, deliberately:
 * `phpize --clean` shells out `rm -f tests/*.php` to wipe the per-test
 * generated PHP files (run-tests.php drops one .php sibling per .phpt
 * during a test run, and phpize clean is meant to wipe those). Any
 * tracked fixture in tests/ that ends in .php gets clobbered along
 * with them. The .inc extension keeps this file outside that glob;
 * PHP doesn't care about the suffix when require()'d by full path.
 *
 * Usage in --SKIPIF--:
 *
 *     require __DIR__ . '/_font_candidates.inc';
 *     if (fc_pick_font() === '') echo "skip: no system font present\n";
 *
 * Usage in --FILE--:
 *
 *     require __DIR__ . '/_font_candidates.inc';
 *     $font = fc_pick_font();
 *     if ($font === '') die("skip: no system font found\n");
 */
function fc_pick_font(): string
{
    $candidates = [
        getenv('FC_FONT') ?: '',
        /* Lato preferred: UI-grade sans, hints cleanly at axis-label
         * sizes. Layered by distro packaging path. */
        '/usr/share/fonts/truetype/lato/Lato-Regular.ttf',   // Debian / Ubuntu (fonts-lato)
        '/usr/share/fonts/lato-fonts/Lato-Regular.ttf',      // Fedora / RHEL (lato-fonts)
        '/usr/share/fonts/TTF/Lato-Regular.ttf',             // Arch
        /* DejaVu fallback: present on most Linux base installs. */
        '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',   // Debian / Ubuntu (fonts-dejavu)
        '/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf', // Fedora / RHEL / CentOS (dejavu-sans-fonts)
        '/usr/share/fonts/dejavu/DejaVuSans.ttf',            // some RPM layouts
        '/usr/share/fonts/TTF/DejaVuSans.ttf',               // Arch
    ];
    foreach ($candidates as $cand) {
        if ($cand !== '' && is_readable($cand)) {
            return $cand;
        }
    }
    return '';
}
