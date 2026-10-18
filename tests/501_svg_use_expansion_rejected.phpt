--TEST--
SVG conversion rejects deep, fanned-out and ancestor-referencing use graphs hidden behind comment-like attribute bytes
--EXTENSIONS--
fastchart
--FILE--
<?php
$h = '<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64">'
   . '<rect id="g<!--" width="0" height="0"/>';
$t = '<rect id="-->" width="0" height="0"/></svg>';

$chain = '<defs><rect id="u30000" width="1" height="1" fill="red"/>';
for ($i = 29999; $i >= 1; $i--) {
    $chain .= "<use id=\"u$i\" href=\"#u" . ($i + 1) . '"/>';
}
$chain .= '</defs><use href="#u1"/>';

$fan = '<defs><rect id="l0" width="64" height="64" fill="red"/>';
for ($i = 1; $i <= 24; $i++) {
    $fan .= "<g id=\"l$i\"><use href=\"#l" . ($i - 1) . "\"/><use href=\"#l" . ($i - 1) . '"/></g>';
}
$fan .= '</defs><use href="#l24"/>';

$cases = [
    'chain' => $chain,
    'fanout' => $fan,
    'ancestor' => '<g id="a"><rect width="8" height="8" fill="red"/><use href="#a" x="8"/></g>',
    'self' => '<use id="s" href="#s"/>',
    'symbol_cycle' => '<symbol id="p"><use href="#q"/></symbol><symbol id="q"><use href="#p"/></symbol><use href="#p"/>',
];
foreach (['svgToPng', 'svgToJpeg', 'svgToWebp'] as $m) {
    foreach ($cases as $name => $body) {
        $start = hrtime(true);
        try {
            FastChart\Chart::$m($h . $body . $t);
            echo "$m $name: accepted\n";
        } catch (ValueError $e) {
            $fast = (hrtime(true) - $start) < 5e9;
            echo "$m $name: ", str_contains($e->getMessage(), 'must not contain <use>')
                ? 'rejected' : 'wrong message', $fast ? '' : ' (slow)', "\n";
        }
    }
}
?>
--EXPECT--
svgToPng chain: rejected
svgToPng fanout: rejected
svgToPng ancestor: rejected
svgToPng self: rejected
svgToPng symbol_cycle: rejected
svgToJpeg chain: rejected
svgToJpeg fanout: rejected
svgToJpeg ancestor: rejected
svgToJpeg self: rejected
svgToJpeg symbol_cycle: rejected
svgToWebp chain: rejected
svgToWebp fanout: rejected
svgToWebp ancestor: rejected
svgToWebp self: rejected
svgToWebp symbol_cycle: rejected
