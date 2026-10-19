--TEST--
Strokes with coordinates far outside the canvas stay on their visible path
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php

function render(string $body, int $size = 64)
{
    return imagecreatefromstring(FastChart\Chart::svgToPng(
        '<svg xmlns="http://www.w3.org/2000/svg" width="' . $size
        . '" height="' . $size . '">' . $body . '</svg>'));
}

function alpha_delta(string $a, string $b): int
{
    $ia = render($a);
    $ib = render($b);
    $max = 0;
    for ($y = 0; $y < 64; $y++) {
        for ($x = 0; $x < 64; $x++) {
            $d = abs(((imagecolorat($ia, $x, $y) >> 24) & 0x7F)
                - ((imagecolorat($ib, $x, $y) >> 24) & 0x7F));
            $max = max($max, $d);
        }
    }
    return $max;
}

function inked($im, int $x, int $y): bool
{
    return ((imagecolorat($im, $x, $y) >> 24) & 0x7F) < 0x40;
}

/* Both ends far away; the clipped segment must keep the slope. */
echo 'line through canvas: ', alpha_delta(
    '<path d="M-1e30 -3e29 L1e30 3e29" fill="none" stroke="black" stroke-width="3"/>',
    '<path d="M-10000 -3000 L10000 3000" fill="none" stroke="black" stroke-width="3"/>')
    <= 1 ? "matches\n" : "differs\n";
echo 'steep line through canvas: ', alpha_delta(
    '<path d="M-3e29 -1e30 L3e29 1e30" fill="none" stroke="black" stroke-width="3"/>',
    '<path d="M-3000 -10000 L3000 10000" fill="none" stroke="black" stroke-width="3"/>')
    <= 1 ? "matches\n" : "differs\n";

/* A cubic pulled toward 1e30 leaves (8,56) heading up and to the right. */
$cubic = render('<path d="M8 56 C 1e30 -1e30 -1e30 1e30 56 8" fill="none" stroke="black" stroke-width="2"/>');
echo 'cubic leaves along its tangent: ',
    (inked($cubic, 20, 44) && !inked($cubic, 20, 20)) ? "yes\n" : "NO\n";

/* Thousands of huge stroked cubics and circles stay cheap and drawable. */
$d = ['M0 0'];
for ($i = 0; $i < 3000; $i++) {
    $d[] = sprintf('C 1e30 -1e30 -1e30 1e30 %d %d', $i % 64, $i % 37);
}
$d = implode(' ', $d);
$png = FastChart\Chart::svgToPng(
    '<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64">'
    . '<path d="' . $d . '" stroke="black" fill="none"/></svg>');
echo 'many huge cubics: ', imagecreatefromstring($png) ? "rendered\n" : "NO\n";

$circles = render('<g stroke="black" stroke-width="3e38">'
    . str_repeat('<circle cx="32" cy="32" r="3e38"/>', 200) . '</g>', 1);
echo 'huge circles fill canvas: ', inked($circles, 0, 0) ? "yes\n" : "NO\n";

?>
--EXPECT--
line through canvas: matches
steep line through canvas: matches
cubic leaves along its tangent: yes
many huge cubics: rendered
huge circles fill canvas: yes
