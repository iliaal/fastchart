--TEST--
CalendarHeatmap: oversized date span rejected (regression: render-cost DoS)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: render cost (cells emitted, civil_from_days calls, SVG size)
 * scales with the date SPAN, not the entry count. setData's 16384-entry cap
 * does not bound it — two entries a few decades apart pass setData yet force
 * a multi-million-cell render. The span is now capped at
 * FASTCHART_MAX_CALENDAR_WEEKS (2400 ≈ 46 yrs). */

$wide = (new FastChart\CalendarHeatmap())->setSize(400, 200);
$wide->setData(['1970-01-01' => 1.0, '2040-12-31' => 2.0]);  /* 2 entries, ~70 yr span */
try {
    $wide->renderSvg();
    echo "wide_span: NOT REJECTED (BAD)\n";
} catch (\Error $e) {
    echo "wide_span: ",
        (strpos($e->getMessage(), 'weeks') !== false ? 'rejected' : 'wrong msg: ' . $e->getMessage()),
        "\n";
}

/* A normal multi-month calendar still renders. */
$ok = (new FastChart\CalendarHeatmap())->setSize(400, 200);
$ok->setData(['2024-01-01' => 1.0, '2024-06-15' => 3.0, '2024-12-31' => 2.0]);
echo "normal: ", (strpos($ok->renderSvg(), '<rect') !== false ? 'renders' : 'BAD'), "\n";
echo "done\n";
?>
--EXPECT--
wide_span: rejected
normal: renders
done
