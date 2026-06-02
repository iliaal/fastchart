--TEST--
Calendar-aware stride: extreme candle timestamps don't read an indeterminate struct tm
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_7ec12b11 — fastchart_draw_x_axis_time calendar-stride branch.
 *
 * setDateAxisStride() enables the calendar-aware tick path, which
 * breaks t_min down with gmtime_r before snapping to a unit boundary.
 * gmtime_r returns NULL when the year overflows struct tm's int
 * tm_year. Candle timestamps arrive unclamped from setOhlcv, so a
 * PHP_INT_MAX timestamp left tm_buf indeterminate and the subsequent
 * timegm()/strftime() read uninitialized members — UB (UBSan abort on
 * CI builds, garbage ticks otherwise). The numeric/auto path below
 * already clamped; the calendar branch did not.
 *
 * Fix: fc_gmtime returns success; on failure the calendar branch is
 * skipped and rendering falls through to the numeric auto-density
 * path, whose own gmtime failures fall back to a raw-integer label.
 *
 * Discriminator: extreme timestamps + a stride render well-formed SVG
 * (no UB / no UBSan abort), and in NATIVE text mode the fallback emits
 * the raw integer label rather than a strftime'd garbage date. */

/* Case 1: t_min == PHP_INT_MAX. gmtime_r fails; calendar branch must
 * fall through. Without the fix this reads an indeterminate struct tm. */
foreach ([
    'DAY'     => FastChart\Chart::DATE_DAY,
    'WEEK'    => FastChart\Chart::DATE_WEEK,
    'MONTH'   => FastChart\Chart::DATE_MONTH,
    'QUARTER' => FastChart\Chart::DATE_QUARTER,
    'YEAR'    => FastChart\Chart::DATE_YEAR,
] as $name => $unit) {
    $svg = (new FastChart\StockChart(600, 300))
        ->setOhlcv([
            [PHP_INT_MAX - 1, 100.0, 110.0, 90.0, 105.0, 1000.0],
            [PHP_INT_MAX,     101.0, 111.0, 91.0, 106.0, 1000.0],
        ])
        ->setDateAxisStride($unit, 1)
        ->renderSvg();
    echo "$name: ",
        (strlen($svg) > 500 && strpos($svg, '</svg>') !== false ? "ok" : "BAD"),
        "\n";
}

/* Case 2: same extreme input in NATIVE text mode. On 64-bit PHP the
 * timestamp overflows struct tm, so the numeric fallback emits the raw
 * integer as the label (a long digit run inside a <text>) — this is the
 * discriminator that catches the unfixed indeterminate-tm read. On
 * 32-bit PHP zend_long == time_t == 32-bit, so PHP_INT_MAX is a valid
 * 2038 timestamp and the overflow is unreachable; assert clean render. */
$svg2 = (new FastChart\StockChart(600, 300))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setOhlcv([
        [PHP_INT_MAX - 1, 100.0, 110.0, 90.0, 105.0, 1000.0],
        [PHP_INT_MAX,     101.0, 111.0, 91.0, 106.0, 1000.0],
    ])
    ->setDateAxisStride(FastChart\Chart::DATE_DAY, 1)
    ->renderSvg();
$fallback_ok = PHP_INT_SIZE >= 8
    ? (bool) preg_match('/<text[^>]*>[^<]*\d{16,}/', $svg2)
    : (strlen($svg2) > 500 && strpos($svg2, '</svg>') !== false);
echo "extreme_numeric_fallback: ", ($fallback_ok ? "ok" : "BAD"), "\n";

/* Sanity: normal-range timestamps still take the calendar path and
 * emit real date labels. 1700000000 == 2023-11-14 UTC. */
$rows = [];
for ($i = 0; $i < 60; $i++) {
    $rows[] = [1700000000 + $i * 86400, 100.0, 102.0, 99.0, 101.0, 1000.0];
}
$svg3 = (new FastChart\StockChart(700, 400))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setOhlcv($rows)
    ->setDateAxisStride(FastChart\Chart::DATE_MONTH, 1)
    ->renderSvg();
echo "normal_date_labels: ",
    (strpos($svg3, '>2023-1') !== false || strpos($svg3, '>2023-12') !== false
        ? "ok" : "BAD"), "\n";

echo "done\n";
?>
--EXPECT--
DAY: ok
WEEK: ok
MONTH: ok
QUARTER: ok
YEAR: ok
extreme_numeric_fallback: ok
normal_date_labels: ok
done
