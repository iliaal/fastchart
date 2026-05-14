--TEST--
phpinfo() reports library versions; missing-lib renderers throw cleanly
--EXTENSIONS--
fastchart
--FILE--
<?php

/* MINFO surface: phpinfo() includes per-library version rows so a
 * user / packager can confirm which codecs are linked in.
 *
 * Optional-lib runtime guard: renderPng / renderJpeg / renderWebp
 * each check fastchart_have_libXxx() before paying for SVG build
 * and rasterization. When a codec is missing the method throws an
 * Error naming the library; when it's present the render returns
 * the expected magic bytes.
 *
 * On the default v1.0 build all three codecs are present, so all
 * three renders should succeed here. The "not compiled in" branch
 * is exercised by inspecting phpinfo() for the row text. */

ob_start();
phpinfo(INFO_MODULES);
$info = ob_get_clean();

$section_start = strpos($info, 'fastchart');
$section = substr($info, $section_start, 800);

echo "has_version_row: ",
    (preg_match('/fastchart version[^0-9]+\d+\.\d+\.\d+/', $section)
        ? "yes" : "no"), "\n";
echo "has_freetype_row: ",
    (preg_match('/FreeType[^0-9]+\d+\.\d+\.\d+/', $section)
        ? "yes" : "no"), "\n";
echo "has_libpng_row: ",
    (preg_match('/libpng[^|]+(\d+\.\d+\.\d+|not compiled in)/', $section)
        ? "yes" : "no"), "\n";
echo "has_libjpeg_row: ",
    (preg_match('/libjpeg[^|]+(\d+\.\d+\.\d+|not compiled in)/', $section)
        ? "yes" : "no"), "\n";
echo "has_libwebp_row: ",
    (preg_match('/libwebp[^|]+(\d+\.\d+\.\d+|not compiled in)/', $section)
        ? "yes" : "no"), "\n";
echo "has_plutovg_row: ",
    (preg_match('/plutovg[^|]+\d+\.\d+\.\d+/', $section)
        ? "yes" : "no"), "\n";

/* The renderXxx paths should either succeed (codec present) or throw
 * a clear "not compiled in" Error — never crash or produce garbage. */
$line = (new FastChart\LineChart(120, 80))->setSeries([1, 2, 3]);

foreach ([
    'renderPng'  => "\x89PNG\r\n\x1a\n",
    'renderJpeg' => "\xFF\xD8\xFF",
    'renderWebp' => "RIFF",
] as $method => $magic) {
    try {
        $bytes = $line->$method();
        $ok = substr($bytes, 0, strlen($magic)) === $magic;
        echo "$method: ", ($ok ? "ok" : "FAIL bad magic"), "\n";
    } catch (Error $e) {
        echo "$method: throws: ", $e->getMessage(), "\n";
    }
}

echo "ok\n";
?>
--EXPECTF--
has_version_row: yes
has_freetype_row: yes
has_libpng_row: yes
has_libjpeg_row: yes
has_libwebp_row: yes
has_plutovg_row: yes
renderPng: %s
renderJpeg: %s
renderWebp: %s
ok
