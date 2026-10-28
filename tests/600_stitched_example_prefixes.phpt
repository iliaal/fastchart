--TEST--
Stitched SVG documentation examples keep fragment definitions distinct
--EXTENSIONS--
fastchart
--FILE--
<?php
$root = dirname(__DIR__);
// Git may check documentation out with CRLF on Windows.
$guide = str_replace("\r\n", "\n", file_get_contents($root . '/docs/README.md'));
$claim = 'one image, stitch SVG fragments with a distinct `drawSvgFragment($idPrefix)`' . "\n"
    . 'for each chart so gradient and clip-path definitions cannot collide, or' . "\n"
    . 'composite the `renderPng()` bytes in userland.';
echo 'guide contract: ', str_contains($guide, $claim) ? 'yes' : 'NO', "\n";

function sample(string $source, bool $gradient): array {
    // Execute the actual two construction chains, not a copied prefix list.
    preg_match_all('/\$(left_g|right_g|left|right)\s*=\s*\(new FastChart\\\\.*?->drawSvgFragment\([^;]*;/s', $source, $matches, PREG_SET_ORDER);
    $W = 800; $H = 320;
    $fragments = [];
    foreach ($matches as $i => $match) {
        $code = $match[0];
        // Font selection is unrelated to IDs and varies across CI platforms.
        $code = str_replace('->setFontPath($font)', '', $code);
        $code = str_replace('[/* … */]', "[['data' => [3, 7, 5]]]", $code);
        if ($gradient) {
            // Customize the line as a filled area and add gradients to both
            // actual chains, preserving their documented fragment calls.
            $code = str_replace('FastChart\\LineChart', 'FastChart\\AreaChart', $code);
            $colors = $i === 0 ? '0x2266AA, 0x88CCEE' : '0xAA6622, 0xEECC88';
            $code = str_replace('->drawSvgFragment(', '->setGradientFill(' . $colors . ')->drawSvgFragment(', $code);
        }
        eval($code);
        $fragments[] = ${$match[1]};
    }
    return $fragments;
}
function valid(array $fragments, bool $definitions): bool {
    if (count($fragments) !== 2) return false;
    $all = [];
    foreach ($fragments as $fragment) {
        preg_match_all('/\bid="([^"]+)"/', $fragment, $ids);
        preg_match_all('/url\(#([^\)]+)\)|(?:xlink:)?href="#([^"]+)"/', $fragment, $references, PREG_SET_ORDER);
        if ($definitions && (!$ids[1] || !$references)) return false;
        foreach ($references as $reference) {
            $id = $reference[1] !== '' ? $reference[1] : $reference[2];
            if (!in_array($id, $ids[1], true)) return false;
        }
        $all = array_merge($all, $ids[1]);
    }
    return count($all) === count(array_unique($all));
}
foreach ([
    'guide' => $guide,
    'misc' => file_get_contents($root . '/docs/examples/31_misc_options.php'),
    'raster' => file_get_contents($root . '/docs/examples/51_svg_to_raster.php'),
] as $name => $source) {
    echo "$name original: ", valid(sample($source, false), false) ? 'yes' : 'NO', "\n";
    echo "$name gradients: ", valid(sample($source, true), true) ? 'yes' : 'NO', "\n";
}
?>
--EXPECT--
guide contract: yes
guide original: yes
guide gradients: yes
misc original: yes
misc gradients: yes
raster original: yes
raster gradients: yes
