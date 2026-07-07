--TEST--
Chart render-time canvas-too-small throws ValueError (matches Symbol family)
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php

/* The render-time "canvas too small / data doesn't fit" condition threw
 * a plain Error in the Chart families but ValueError in the Symbol
 * family. Unified to ValueError (BC-safe: ValueError extends Error).
 * Catching \ValueError specifically proves the class, since a plain
 * Error would fall through to the \Error arm. */

function which(callable $fn): string {
    try { $fn(); return 'NO THROW'; }
    catch (\ValueError $e) { return 'ValueError'; }
    catch (\Error $e) { return 'Error(' . get_class($e) . ')'; }
}

echo 'funnel: ', which(fn() => (new FastChart\Funnel(150, 300))
    ->setStages([['value' => 10], ['value' => 6]])->renderSvg()), "\n";

echo 'pareto: ', which(fn() => (new FastChart\ParetoChart(100, 300))
    ->setBars([['label' => 'A', 'value' => 5], ['label' => 'B', 'value' => 3]])
    ->renderSvg()), "\n";

/* Wide enough to clear the margin check, but 128 bars (the cap) in a
 * ~100px plot truncate the per-bar slot below 1px — the other
 * doesn't-fit rejection in pareto. */
$nbars = [];
for ($i = 0; $i < 128; $i++) { $nbars[] = ['label' => "n$i", 'value' => $i + 1]; }
echo 'pareto narrow slots: ', which(fn() => (new FastChart\ParetoChart(220, 300))
    ->setBars($nbars)->renderSvg()), "\n";

echo 'marimekko: ', which(fn() => (new FastChart\MarimekkoChart(40, 300))
    ->setColumns([['label' => 'G', 'segments' => [['label' => 'A', 'value' => 10]]]])
    ->renderSvg()), "\n";

$wbars = [];
for ($i = 0; $i < 80; $i++) { $wbars[] = ['label' => "b$i", 'value' => ($i % 2 ? 3 : -2)]; }
echo 'waterfall: ', which(fn() => (new FastChart\Waterfall(90, 300))
    ->setBars($wbars)->renderSvg()), "\n";

echo "done\n";
?>
--EXPECT--
funnel: ValueError
pareto: ValueError
pareto narrow slots: ValueError
marimekko: ValueError
waterfall: ValueError
done
