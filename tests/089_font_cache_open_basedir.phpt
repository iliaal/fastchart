--TEST--
Per-render font cache invalidates at draw entry: open_basedir narrowing between two draws of the same chart object is honored on every renderer
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
/* The test pins to /usr/share fonts because the open_basedir narrowing
 * step needs a path that's reachable before the narrow and outside
 * the narrowed dir afterwards. Windows / macOS hosts don't have
 * /usr/share, so skip cleanly there. */
require __DIR__ . '/_font_candidates.inc.php';
if (fc_pick_font() === '') echo "skip: no system font present\n";
?>
--FILE--
<?php
/* Regression for the open_basedir bypass via the per-render font
 * cache. Pre-fix, non-layout renderers (gauge / radar / polar /
 * surface / contour) only stamped DPI on entry and never invalidated
 * font_cache_valid — so a chart object warmed with a font path that
 * sat outside open_basedir would keep rendering with that path after
 * a runtime ini_set('open_basedir') narrowing.
 *
 * Pick a font under /usr/share that we know is reachable on a
 * default-permissive runtime; if it isn't present, skip — the test
 * is about cache invalidation, not font availability.
 */
require __DIR__ . '/_font_candidates.inc.php';
$font = fc_pick_font();
if ($font === '') { echo "skip: no system font under /usr/share\n"; exit; }

/* Run the same probe across each non-layout family. Each function:
 *   1. constructs a chart with $font (outside the to-be-narrowed
 *      base dir),
 *   2. optionally renders once to warm the font cache,
 *   3. narrows open_basedir to /tmp,
 *   4. renders again and returns the bytes.
 *
 * The warmed and unwarmed bytes MUST match — both should reflect the
 * narrowed policy, not a cached pre-narrowing path. */
function probe_family(string $klass, string $font, bool $warm): string {
    $c = new $klass(220, 140);
    $c->setFontPath($font)->setTitle('t');
    if (method_exists($c, 'setRange')) $c->setRange(0, 100);
    if (method_exists($c, 'setValue')) $c->setValue(50);
    if (method_exists($c, 'setSeries')) $c->setSeries([['data' => [1, 2, 3, 4]]]);
    if (method_exists($c, 'setAxes'))   $c->setAxes(['a','b','c','d']);
    if (method_exists($c, 'setRPoints')) $c->setRPoints([[0, 1], [90, 2], [180, 1.5]]);
    if (method_exists($c, 'setGrid')) {
        $c->setGrid([[1,2,3,4],[2,3,4,5],[3,4,5,6],[4,5,6,7]]);
    }
    if ($warm) { $c->renderPng(); }
    ini_set('open_basedir', '/tmp');
    $bytes = $c->renderPng();
    ini_restore('open_basedir');
    return $bytes;
}

$families = [
    'GaugeChart'   => 'FastChart\\GaugeChart',
    'RadarChart'   => 'FastChart\\RadarChart',
    'PolarChart'   => 'FastChart\\PolarChart',
    'SurfaceChart' => 'FastChart\\SurfaceChart',
    'ContourChart' => 'FastChart\\ContourChart',
];

foreach ($families as $name => $klass) {
    $warm   = @probe_family($klass, $font, true);
    $unwarm = @probe_family($klass, $font, false);
    echo "$name: ", ($warm === $unwarm ? "match" : "DIFFER"), "\n";
}
?>
--EXPECTF--
GaugeChart: match
RadarChart: match
PolarChart: match
SurfaceChart: match
ContourChart: match
