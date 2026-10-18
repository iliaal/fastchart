--TEST--
Fills with coordinates far outside the canvas render like their in-range equivalents
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php

function render(string $body)
{
    return imagecreatefromstring(FastChart\Chart::svgToPng(
        '<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64">'
        . $body . '</svg>'));
}

function same_pixels(string $a, string $b): string
{
    $ia = render($a);
    $ib = render($b);
    $ink = 0;
    for ($y = 0; $y < 64; $y++) {
        for ($x = 0; $x < 64; $x++) {
            $c = imagecolorat($ia, $x, $y);
            if ($c !== imagecolorat($ib, $x, $y)) {
                return "differs at $x,$y";
            }
            if ((($c >> 24) & 0x7F) !== 0x7F) {
                $ink++;
            }
        }
    }
    return $ink > 0 ? 'same' : 'blank';
}

echo 'closed triangle: ', same_pixels(
    '<path d="M8 8 L1e30 3e29 L8 1e30 Z"/>',
    '<path d="M8 8 L10008 3008 L8 10008 Z"/>'), "\n";
echo 'open triangle: ', same_pixels(
    '<path d="M8 8 L1e30 3e29 L8 1e30"/>',
    '<path d="M8 8 L10008 3008 L8 10008"/>'), "\n";
echo 'corner past the edge: ', same_pixels(
    '<path d="M10 10 L1e30 10 L10 40 Z" fill="red"/>',
    '<path d="M10 10 L1e7 10 L10 40 Z" fill="red"/>'), "\n";
echo 'even-odd hole: ', same_pixels(
    '<path fill-rule="evenodd" d="M-1e30 -1e30 L1e30 -1e30 L1e30 1e30 '
    . 'L-1e30 1e30 Z M10 10 L50 10 L50 50 L10 50 Z"/>',
    '<path fill-rule="evenodd" d="M-100 -100 L100 -100 L100 100 L-100 100 Z '
    . 'M10 10 L50 10 L50 50 L10 50 Z"/>'), "\n";
echo 'huge circle covers canvas: ', same_pixels(
    '<circle cx="32" cy="32" r="3e38" stroke="black" stroke-width="3e38"/>',
    '<rect width="64" height="64"/>'), "\n";
echo 'large circle edge: ', same_pixels(
    '<circle cx="1e7" cy="32" r="9999968" fill="red"/>',
    '<rect x="32" width="32" height="64" fill="red"/>'), "\n";

$nan = render('<path d="M0 0 L64 0 L64 1e39 Z"/>');
echo 'infinite path dropped: ',
    ((imagecolorat($nan, 60, 10) >> 24) & 0x7F) === 0x7F ? "yes\n" : "NO\n";

?>
--EXPECT--
closed triangle: same
open triangle: same
corner past the edge: same
even-odd hole: same
huge circle covers canvas: same
large circle edge: same
infinite path dropped: yes
