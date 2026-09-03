--TEST--
Repeated source images share definitions, cached decoding, and placements
--EXTENSIONS--
fastchart
gd
--INI--
asan.detect_leaks=0
--FILE--
<?php

function image_markup(string $svg): array
{
	preg_match_all(
		'/<image id="([^"]*fci\d+)" width="(\d+)" height="(\d+)" '
		. 'preserveAspectRatio="none" href="data:image\/(?:png|jpeg);base64,/',
		$svg,
		$definitionMatches,
		PREG_SET_ORDER
	);
	$dimensions = [];
	$defs = [];
	foreach ($definitionMatches as $definition) {
		$defs[] = $definition[1];
		$dimensions[$definition[1]] = [(int)$definition[2], (int)$definition[3]];
	}
	preg_match_all(
		'/<use href="#([^"]*fci\d+)" x="(-?\d+)" y="(-?\d+)"\/>/',
		$svg,
		$useMatches,
		PREG_SET_ORDER
	);
	$uses = [];
	foreach ($useMatches as $use) {
		[$width, $height] = $dimensions[$use[1]];
		$uses[] = [$use[0], $use[1], $use[2], $use[3], $width, $height];
	}
	return [$defs, $uses];
}

function is_red_at($im, array $use): bool
{
    $x = (int)$use[2] + intdiv((int)$use[4], 2);
    $y = (int)$use[3] + intdiv((int)$use[5], 2);
    $c = imagecolorat($im, $x, $y);
    return (($c >> 16) & 0xFF) > 220
        && (($c >> 8) & 0xFF) < 40
        && ($c & 0xFF) < 40;
}

$red = __DIR__ . '/__icon.png';
$blue = __DIR__ . '/__icon82.png';
$data = array_fill(0, 32, 50.0);
$one = (new FastChart\LineChart(960, 360))->setSeries($data);
for ($i = 0; $i < 32; $i++) {
    $one->addIconAt((float)$i, 50.0, $red);
}

$one_svg = $one->renderSvg();
[$one_defs, $one_uses] = image_markup($one_svg);
echo 'one-path definitions: ', count($one_defs), "\n";
echo 'one-path data uris: ', substr_count($one_svg, 'data:image/'), "\n";
echo 'one-path uses: ', count($one_uses), "\n";
echo 'one-path shared reference: ',
    (count(array_unique(array_column($one_uses, 1))) === 1
     && $one_defs === ['fci1']) ? "yes\n" : "NO\n";
echo 'repeat render stable: ',
    $one->renderSvg() === $one_svg ? "yes\n" : "NO\n";

$png = $one->renderPng();
$raster = imagecreatefromstring($png);
echo 'raster output: ',
    ($raster && imagesx($raster) === 960 && imagesy($raster) === 360)
        ? "yes\n" : "NO\n";
echo 'first and last use red: ',
    ($raster && is_red_at($raster, $one_uses[0])
     && is_red_at($raster, $one_uses[31])) ? "yes\n" : "NO\n";

$two = (new FastChart\LineChart(320, 200))
    ->setSeries([1, 2, 3])
    ->addIconAt(0.0, 1.0, $red)
    ->addIconAt(2.0, 3.0, $blue);
[$two_defs, $two_uses] = image_markup($two->renderSvg());
echo 'two-path definitions: ', count($two_defs), "\n";
echo 'two-path references: ',
    (count(array_unique(array_column($two_uses, 1))) === 2)
        ? "yes\n" : "NO\n";

$mixed_size_svg = (new FastChart\LineChart(320, 200))
	->setSeries([1, 2, 3])
	->addIconAt(0.0, 1.0, $red, 14, 14)
	->addIconAt(2.0, 3.0, $red, 15, 15)
	->renderSvg();
echo 'mixed-size scale exact: ',
	str_contains($mixed_size_svg,
		'scale(1.0714285714285714 1.0714285714285714)')
		? "yes\n" : "NO\n";

$aspect_path = tempnam(sys_get_temp_dir(), 'fc_icon_aspect_');
$aspect_image = imagecreatetruecolor(40, 20);
imagefill($aspect_image, 0, 0, imagecolorallocate($aspect_image, 0, 220, 0));
imagepng($aspect_image, $aspect_path);
$aspect_svg = (new FastChart\LineChart(320, 200))
    ->setSeries([1, 2, 3])
    ->addIconAt(1.0, 2.0, $aspect_path, 20, 20)
    ->renderSvg();
[, $aspect_uses] = image_markup($aspect_svg);
echo 'aspect preserved: ',
    (count($aspect_uses) === 1
     && (int)$aspect_uses[0][4] === 20
     && (int)$aspect_uses[0][5] === 10) ? "yes\n" : "NO\n";
@unlink($aspect_path);

$fragment = $one->drawSvgFragment('icons');
[$fragment_defs, $fragment_uses] = image_markup($fragment);
echo 'fragment namespace: ',
    ($fragment_defs === ['iconsfci1']
     && count(array_unique(array_column($fragment_uses, 1))) === 1
     && $fragment_uses[0][1] === 'iconsfci1'
     && !str_contains($fragment, 'href="#fci')) ? "yes\n" : "NO\n";

$missing = tempnam(sys_get_temp_dir(), 'fc_icon_missing_');
@unlink($missing);
$invalid = tempnam(sys_get_temp_dir(), 'fc_icon_invalid_');
file_put_contents($invalid, 'not an image');
$failures = (new FastChart\LineChart(640, 240))->setSeries($data);
for ($i = 0; $i < 16; $i++) {
    $failures->addIconAt((float)$i, 50.0, $missing);
}
for ($i = 16; $i < 32; $i++) {
    $failures->addIconAt((float)$i, 50.0, $invalid);
}
[$failed_defs, $failed_uses] = image_markup($failures->renderSvg());
echo 'failed paths suppressed: ',
    (!$failed_defs && !$failed_uses) ? "yes\n" : "NO\n";

copy($red, $missing);
copy($blue, $invalid);
[$recovered_defs, $recovered_uses] = image_markup($failures->renderSvg());
echo 'failure cache is per target: ',
    (count($recovered_defs) === 2 && count($recovered_uses) === 32)
        ? "yes\n" : "NO\n";
@unlink($missing);
@unlink($invalid);

try {
    FastChart\Chart::svgToPng($one_svg);
    echo "user svg accepted: NO\n";
} catch (ValueError $e) {
    echo 'user svg rejected: ',
        str_contains($e->getMessage(), 'data:image/') ? "yes\n" : "wrong\n";
}

?>
--EXPECT--
one-path definitions: 1
one-path data uris: 1
one-path uses: 32
one-path shared reference: yes
repeat render stable: yes
raster output: yes
first and last use red: yes
two-path definitions: 2
two-path references: yes
mixed-size scale exact: yes
aspect preserved: yes
fragment namespace: yes
failed paths suppressed: yes
failure cache is per target: yes
user svg rejected: yes
