--TEST--
CalendarHeatmap month labels respect clipped weeks and Gregorian boundaries
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
$svg = (new FastChart\CalendarHeatmap(400, 160))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setData(['2026-01-01' => 1])->renderSvg();
if (!str_contains($svg, '>M</text>')) die('skip no usable default font');
?>
--FILE--
<?php
function checkLabels(string $first, string $last): void {
    $start = new DateTimeImmutable($first, new DateTimeZone('UTC'));
    $end = new DateTimeImmutable($last, new DateTimeZone('UTC'));
    $grid = $start->modify('-' . $start->format('w') . ' days');
    $weeks = intdiv((int) $grid->diff($end)->days + 7, 7);
    $cell = min(intdiv(352, $weeks), 16);
    $expected = [];
    for ($day = $start; $day <= $end; $day = $day->modify('+1 day')) {
        if ($day->format('d') === '01') {
            $column = intdiv((int) $grid->diff($day)->days, 7);
            $expected[] = [strval(36 + $column * $cell), $day->format('M')];
        }
    }
    $chart = (new FastChart\CalendarHeatmap(400, 160))
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setData([$last => 2, $first => 1]);
    $svg = $chart->renderSvg();
    preg_match_all('/<text x="([0-9]+)"[^>]*>(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)<\/text>/', $svg, $matches, PREG_SET_ORDER);
    $actual = array_map(fn($m) => [$m[1], $m[2]], $matches);
    if ($actual !== $expected || $svg !== (clone $chart)->renderSvg()) {
        throw new Exception("Month labels differ for $first through $last");
    }
}
$cases = 0;
foreach (['0001', '1600', '1900', '1969', '1970', '2000', '2026', '9999'] as $year) {
    foreach ([['01-01', '01-01'], ['01-02', '01-02'], ['01-02', '01-07'],
              ['01-28', '02-01'], ['01-28', '02-02'], ['02-02', '03-05'],
              ['02-28', '03-07'], ['12-25', '12-31']] as [$first, $last]) {
        checkLabels("$year-$first", "$year-$last");
        $cases++;
    }
}
// Every possible weekday for a month start, including clipped leading/trailing weeks.
foreach (range(1, 12) as $month) {
    $start = new DateTimeImmutable(sprintf('2026-%02d-01', $month), new DateTimeZone('UTC'));
    foreach ([-3, 0, 1] as $offset) {
        checkLabels($start->modify("$offset days")->format('Y-m-d'), $start->modify('+5 days')->format('Y-m-d'));
        $cases++;
    }
}
checkLabels('1969-12-29', '1970-01-04');
checkLabels('1999-12-29', '2000-03-02');
echo $cases + 2, " month-label cases passed\n";
?>
--EXPECT--
102 month-label cases passed
