<?php
/* Shared WordCloud box measurement for phpts. A word is drawn iff its box
 * fits the plot rectangle (a lone word is tried at the plot centre first),
 * so the smallest plot width and height that still draw it are its box.
 * Two over-long words carry the given lowest and highest weights so the
 * measured word gets the font size it has in the full cloud; they are
 * wider than any plot, so they are never drawn. */
function wc_draws(string $font, int $w, int $h, array $words, int $x1, int $y1): bool
{
    $svg = (new FastChart\WordCloud($w, $h))
        ->setFontPath($font)
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setWords($words)
        ->setPlotRect(0, 0, $x1, $y1)
        ->renderSvg();
    return preg_match('/<text\b/', $svg) === 1;
}

/* [width, height] of $word's box on a $w x $h canvas, or null when it
 * does not fit the whole canvas. */
function wc_box(string $font, int $w, int $h, array $word, float $lo, float $hi): ?array
{
    $pad = str_repeat('W', 400);
    $words = [['text' => $pad, 'weight' => $lo], ['text' => $pad, 'weight' => $hi], $word];
    if (!wc_draws($font, $w, $h, $words, $w - 1, $h - 1)) return null;
    $size = [];
    foreach ([0, 1] as $axis) {
        $a = 0;
        $b = $axis === 0 ? $w - 1 : $h - 1;
        while ($a < $b) {
            $m = intdiv($a + $b, 2);
            $ok = $axis === 0 ? wc_draws($font, $w, $h, $words, $m, $h - 1)
                              : wc_draws($font, $w, $h, $words, $w - 1, $m);
            if ($ok) $b = $m; else $a = $m + 1;
        }
        $size[] = $a;
    }
    return $size;
}
