--TEST--
Chart::renderToFile() routes .svg to the vector backend
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php
$tmp = tempnam(sys_get_temp_dir(), "fastchart_svg_") . ".svg";

try {
    $c = new FastChart\LineChart(480, 240);
    $c->setSeries([10, 22, 17, 28, 24, 30])
      ->setTitle("toFile");

    $written = $c->renderToFile($tmp);
    var_dump(is_int($written));
    var_dump($written > 200);
    var_dump(file_exists($tmp));
    var_dump(filesize($tmp) === $written);

    $svg = file_get_contents($tmp);
    var_dump(str_starts_with($svg, "<?xml "));
    var_dump(strpos($svg, "</svg>") !== false);
    var_dump(simplexml_load_string($svg) instanceof SimpleXMLElement);

    // Case-insensitive extension match: .SVG also works.
    $tmp_upper = $tmp . "x.SVG";
    $written2 = $c->renderToFile($tmp_upper);
    var_dump($written2 > 0);
    var_dump(file_get_contents($tmp_upper) === $svg);
    unlink($tmp_upper);

    // All chart families now support SVG output. ScatterChart is one
    // of the post-pilot families that landed in the second wave.
    $sc = new FastChart\ScatterChart(200, 200);
    $sc->setPoints([[1.0, 2.0], [3.0, 4.0]]);
    $sc->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
    $sc->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE);
    $sc_svg = $sc->renderSvg();
    var_dump(str_starts_with($sc_svg, "<?xml "));
    var_dump(strpos($sc_svg, "</svg>") !== false);

    echo "OK\n";
} finally {
    @unlink($tmp);
}
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
OK
