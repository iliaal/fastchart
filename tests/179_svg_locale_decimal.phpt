--TEST--
SVG geometry uses '.' decimal under any LC_NUMERIC (regression: comma-decimal corruption)
--SKIPIF--
<?php
if (@setlocale(LC_NUMERIC, 'de_DE.UTF-8', 'de_DE', 'de_DE.utf8', 'fr_FR.UTF-8', 'fr_FR', 'nl_NL.UTF-8') === false)
    die('skip no comma-decimal locale installed');
?>
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Regression: fc_svg_fmt_num / fc_emit_num / fc_svg_fmt_color formatted SVG
 * coordinates with snprintf("%f"), which honours LC_NUMERIC. Under a
 * comma-decimal locale (de_DE, fr_FR, ...) that emitted width="307,0",
 * x="69,5" etc. — and a comma is SVG's own coordinate separator, so the
 * geometry was silently corrupted for every consumer (browser, plutovg).
 * Geometry must always use '.'; only human-readable LABEL text may localise.
 * Pre-fix this exact chart produced 118 comma-decimal coordinate attributes. */

function bar_svg() {
    return (new FastChart\BarChart(307, 211))    /* 307/3 forces fractional bar geometry */
        ->setSeries([['data' => [1.5, 2.5, 3.25]]])
        ->renderSvg();
}

/* A numeric attribute whose value is digits-comma-digit (e.g. x="69,5") is a
 * corrupted coordinate. BarChart emits no `points=`/path-with-leading-digit
 * attributes, so this pattern only matches the decimal-separator bug. */
$re = '/="-?[0-9]+,[0-9]/';

setlocale(LC_NUMERIC, 'C');
echo "c_locale_comma_coords: ", preg_match_all($re, bar_svg()), "\n";

setlocale(LC_NUMERIC, 'de_DE.UTF-8', 'de_DE', 'de_DE.utf8', 'fr_FR.UTF-8', 'fr_FR', 'nl_NL.UTF-8');
$de = bar_svg();
echo "de_locale_comma_coords: ", preg_match_all($re, $de), "\n";
echo "de_rendered: ", (strpos($de, '<rect') !== false ? 'ok' : 'BAD'), "\n";
echo "done\n";
?>
--EXPECT--
c_locale_comma_coords: 0
de_locale_comma_coords: 0
de_rendered: ok
done
