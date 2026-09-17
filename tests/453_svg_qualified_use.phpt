--TEST--
SVG conversion rejects qualified use elements without rejecting other local names
--EXTENSIONS--
fastchart
--FILE--
<?php
$open = '<svg xmlns="http://www.w3.org/2000/svg" xmlns:s="http://www.w3.org/2000/svg" width="10" height="10">';
foreach (['svgToPng', 'svgToJpeg', 'svgToWebp'] as $method) {
    foreach (['<s:use/>', '<s:USE/>', '<!-- ignored --><s:use/>'] as $element) {
        try {
            FastChart\Chart::$method($open . $element . '</svg>');
            echo "unexpected acceptance\n";
        } catch (Throwable $e) {
            echo $method, ': ', $e::class, "\n";
        }
    }
}
$plain = $open . '<rect width="10" height="10" fill="red"/></svg>';
$expected = FastChart\Chart::svgToPng($plain);
foreach (['<s:userdata/>', '<use:rect xmlns:use="urn:example"/>', '<!-- <s:use/> -->', '<![CDATA[<s:use/>]]>'] as $element) {
    $actual = FastChart\Chart::svgToPng($open . $element . '<rect width="10" height="10" fill="red"/></svg>');
    echo $actual === $expected ? "render preserved\n" : "render changed\n";
}
?>
--EXPECT--
svgToPng: ValueError
svgToPng: ValueError
svgToPng: ValueError
svgToJpeg: ValueError
svgToJpeg: ValueError
svgToJpeg: ValueError
svgToWebp: ValueError
svgToWebp: ValueError
svgToWebp: ValueError
render preserved
render preserved
render preserved
render preserved
