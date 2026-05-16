<?php
/* Gallery summary reporter. Loads the shared case list from
 * scripts/_gallery_cases.inc.php, renders each chart in all four
 * formats, and prints aggregate size + per-case format breakdown.
 * The HTML-emitting counterpart is scripts/build-v1-gallery.php,
 * which loads the same case list.
 *
 * Run with the fastchart extension loaded — see CLAUDE.md for the
 * explicit -d extension= invocation. */

if (!class_exists('FastChart\\Chart')) {
    fwrite(STDERR, "FastChart not loaded; load fastchart.so + gd.so.\n");
    exit(1);
}

$bundle = require __DIR__ . '/_gallery_cases.inc.php';
$cases  = $bundle['cases'];

/* ----------- Print summary stats ----------- */

$total_svg = 0;
$total_png = 0;
$total_jpg = 0;
$total_webp = 0;
$rows = '';

foreach ($cases as $i => $case) {
    $c = $case['build']();
    $svg  = $c->renderSvg();
    $png  = $c->renderPng();
    $jpg  = $c->renderJpeg();
    $webp = $c->renderWebp();

    $total_svg  += strlen($svg);
    $total_png  += strlen($png);
    $total_jpg  += strlen($jpg);
    $total_webp += strlen($webp);

    $rows .= sprintf("  %2d. %-65s  svg=%6.1f  png=%6.1f  jpg=%6.1f  webp=%6.1f kB\n",
        $i + 1,
        substr($case['label'], 0, 65),
        strlen($svg)  / 1024,
        strlen($png)  / 1024,
        strlen($jpg)  / 1024,
        strlen($webp) / 1024);
}

echo $rows;
echo str_repeat('-', 100), "\n";
printf("  totals (%d cases):                                              "
      ."         svg=%6.1f  png=%6.1f  jpg=%6.1f  webp=%6.1f kB\n",
    count($cases),
    $total_svg  / 1024,
    $total_png  / 1024,
    $total_jpg  / 1024,
    $total_webp / 1024);
