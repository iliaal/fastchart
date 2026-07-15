--TEST--
svgToPng rejects valid SVG whose element count exceeds the parser budget
--EXTENSIONS--
fastchart
--FILE--
<?php

$svg = '<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10">'
    . str_repeat('<g/>', 70000)
    . '</svg>';

try {
    FastChart\Chart::svgToPng($svg);
    echo "element count: accepted\n";
} catch (ValueError $e) {
    echo "element count: rejected\n";
}

$deep = '<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10">'
    . str_repeat('<g>', 300)
    . str_repeat('</g>', 300)
    . '</svg>';
try {
    FastChart\Chart::svgToPng($deep);
    echo "depth: accepted\n";
} catch (ValueError $e) {
    echo "depth: rejected\n";
}
?>
--EXPECT--
element count: rejected
depth: rejected
