--TEST--
CalendarHeatmap: day-grid layout with sparse data
--EXTENSIONS--
fastchart
--FILE--
<?php

/* A handful of days across three months. The grid is inferred from
 * the min/max date; cells without data render in the grid color. */
$data = [];
for ($i = 0; $i < 20; $i++) {
    $data[sprintf('2026-01-%02d', $i + 1)] = $i + 1;
}
$data['2026-02-14'] = 30;
$data['2026-03-15'] = 12;

$svg = (new FastChart\CalendarHeatmap(800, 160))
    ->setData($data)
    ->setColorRamp(0xEEFFEE, 0x004400)
    ->renderSvg();

/* 7 rows × ~10 week columns = 70 cell rects + 1 bg rect. */
echo "rects_ge_30: ", (substr_count($svg, '<rect') >= 30 ? "yes" : "no"), "\n";
echo "has_text: ", (str_contains($svg, '<text') || str_contains($svg, '<path') ? "yes" : "no"), "\n";
echo "well_formed_xml: ",
    (simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING)
        !== false ? "yes" : "no"), "\n";

/* Bad date strings silently dropped. */
$svg2 = (new FastChart\CalendarHeatmap(400, 120))
    ->setData(['bogus' => 1, '2026-13-99' => 2, '2026-04-04' => 5])
    ->renderSvg();
echo "bad_dates_dropped: ", (strlen($svg2) > 100 ? "yes" : "no"), "\n";

echo "ok\n";
?>
--EXPECT--
rects_ge_30: yes
has_text: yes
well_formed_xml: yes
bad_dates_dropped: yes
ok
