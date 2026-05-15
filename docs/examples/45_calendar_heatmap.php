<?php
/* CalendarHeatmap: GitHub-style activity grid. Seven day-of-week
 * rows × N week columns; each cell colored by value via low/high
 * ramp. Missing days render in the palette grid color. */

require __DIR__ . '/_bootstrap.php';

/* Synthetic year of commit-style activity: weekday bursts + sparse
 * weekends + a holiday lull at year-end. */
$data = [];
$start = strtotime('2025-05-01');
$end   = strtotime('2026-04-30');
for ($ts = $start; $ts <= $end; $ts += 86400) {
    $iso = date('Y-m-d', $ts);
    $dow = (int)date('w', $ts);
    $base = $dow >= 1 && $dow <= 5 ? 6 : 1;
    $burst = sin($ts / 86400 / 7) * 4 + cos($ts / 86400 / 3) * 3;
    $v = max(0, $base + $burst + ((int)date('z', $ts) % 11 === 0 ? 8 : 0));
    /* Holiday lull mid-Dec to early-Jan. */
    if ((date('m', $ts) === '12' && (int)date('d', $ts) >= 20)
        || (date('m', $ts) === '01' && (int)date('d', $ts) <= 3)) {
        $v *= 0.2;
    }
    if ($v > 0) $data[$iso] = round($v);
}

(new FastChart\CalendarHeatmap(900, 170))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Daily commits, last 12 months')
    ->setData($data)
    ->setColorRamp(0xEBEDEF, 0x216E39)
    ->renderToFile(__DIR__ . '/45_calendar_heatmap.png');
