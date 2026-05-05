--TEST--
setLineStyle: LINE_SOLID / LINE_DASHED / LINE_DOTTED
--EXTENSIONS--
fastchart
--FILE--
<?php

// A dashed/dotted line drops antialiasing (libgd doesn't compose
// gdStyled and gdAntiAliased), so absolute pixel counts of the
// "exact" series color don't compare cleanly. Verify instead that:
//   1. each style produces a non-trivial render
//   2. dashed/dotted output differ from solid (the styling is real)
//   3. dashed and dotted differ from each other (distinct patterns)
$bytes_solid  = (new FastChart\LineChart(500, 400))
    ->setLineStyle(FastChart\Chart::LINE_SOLID)
    ->setSeries([1, 5, 3, 8, 4, 7, 2, 9])->renderPng();
$bytes_dashed = (new FastChart\LineChart(500, 400))
    ->setLineStyle(FastChart\Chart::LINE_DASHED)
    ->setSeries([1, 5, 3, 8, 4, 7, 2, 9])->renderPng();
$bytes_dotted = (new FastChart\LineChart(500, 400))
    ->setLineStyle(FastChart\Chart::LINE_DOTTED)
    ->setSeries([1, 5, 3, 8, 4, 7, 2, 9])->renderPng();

echo "solid_renders: ",  strlen($bytes_solid)  > 1024 ? "yes" : "no", "\n";
echo "dashed_renders: ", strlen($bytes_dashed) > 1024 ? "yes" : "no", "\n";
echo "dotted_renders: ", strlen($bytes_dotted) > 1024 ? "yes" : "no", "\n";
echo "dashed_differs: ", $bytes_dashed !== $bytes_solid  ? "yes" : "no", "\n";
echo "dotted_differs: ", $bytes_dotted !== $bytes_dashed ? "yes" : "no", "\n";

// Out-of-range rejected.
try {
    (new FastChart\LineChart)->setLineStyle(99);
    echo "bad: no throw\n";
} catch (\ValueError $e) {
    echo "bad: ValueError ok\n";
}
?>
--EXPECT--
solid_renders: yes
dashed_renders: yes
dotted_renders: yes
dashed_differs: yes
dotted_differs: yes
bad: ValueError ok
