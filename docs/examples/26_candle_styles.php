<?php
/* Same OHLCV data rendered in every candle style fastchart supports.
 * Pick the style that fits the audience:
 *   - STYLE_CANDLE   (0): standard filled candlesticks (default)
 *   - STYLE_BAR      (1): OHLC bars (open tick left, close tick right)
 *   - STYLE_DIAMOND  (2): diamond-shaped body, useful for sparser data
 *   - STYLE_I_CAP    (3): I-beam / capped-bar style
 *   - STYLE_HOLLOW   (4): hollow up-candles, filled down-candles
 *   - STYLE_VOLUME   (5): equivolume; body width tracks volume
 *   - STYLE_VECTOR   (6): vector / kagi-like climax markers
 */

require __DIR__ . '/_bootstrap.php';

mt_srand(11);
$rows = [];
$base = 1700000000;
$price = 50.0;
for ($i = 0; $i < 30; $i++) {
    $delta = (mt_rand(-100, 100) / 100.0) * 1.6;
    $open  = $price;
    $close = $price + $delta;
    $high  = max($open, $close) + mt_rand(0, 50) / 100.0;
    $low   = min($open, $close) - mt_rand(0, 50) / 100.0;
    /* Vary volume meaningfully so STYLE_VECTOR can distinguish
     * neutral (gray), rising (purple), and climax (lime/fuchsia)
     * bars. A handful of bars are forced to >2x the trailing
     * average to surface climax coloring. */
    $vol = 30000 + mt_rand(0, 15000);
    if (in_array($i, [4, 11, 17, 23, 27], true)) {
        $vol *= 3;          // climax bar
    } elseif (in_array($i, [7, 14, 20], true)) {
        $vol = (int)($vol * 1.7);  // rising bar
    }
    $rows[] = [$base + $i * 86400, $open, $high, $low, $close, $vol];
    $price  = $close;
}

$styles = [
    ['CANDLE',  FastChart\Chart::STYLE_CANDLE,  '26a_candle.png'],
    ['BAR',     FastChart\Chart::STYLE_BAR,     '26b_bar.png'],
    ['DIAMOND', FastChart\Chart::STYLE_DIAMOND, '26c_diamond.png'],
    ['I-CAP',   FastChart\Chart::STYLE_I_CAP,   '26d_icap.png'],
    ['HOLLOW',  FastChart\Chart::STYLE_HOLLOW,  '26e_hollow.png'],
    ['VOLUME',  FastChart\Chart::STYLE_VOLUME,  '26f_volume.png'],
    ['VECTOR',  FastChart\Chart::STYLE_VECTOR,  '26g_vector.png'],
];

foreach ($styles as [$label, $style, $file]) {
    (new FastChart\StockChart(420, 240))
    ->setFontPath($font)
    ->setDpi($dpi)
        ->setTitle("STYLE_$label")
        ->setOhlcv($rows)
        ->setCandleStyle($style)
        /* Stride the date axis to one tick per week. At 420×240
         * with 30 trading days we'd otherwise pack 8+ date labels
         * into a narrow strip and the "2023-11-14" strings would
         * collide. setDateAxisStride is the chart-aware way to
         * thin the labels (rotation just trades one collision for
         * another at 45°). */
        ->setDateAxisStride(FastChart\Chart::DATE_WEEK)
        ->renderToFile(__DIR__ . '/' . $file);
}
