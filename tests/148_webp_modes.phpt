--TEST--
Chart::setWebpMode(): WEBP_DRAWING / WEBP_PHOTO / WEBP_LOSSLESS / WEBP_FAST
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Class constants exposed at the documented values. */
echo "WEBP_DRAWING:  ", FastChart\Chart::WEBP_DRAWING,  "\n";
echo "WEBP_PHOTO:    ", FastChart\Chart::WEBP_PHOTO,    "\n";
echo "WEBP_LOSSLESS: ", FastChart\Chart::WEBP_LOSSLESS, "\n";
echo "WEBP_FAST:     ", FastChart\Chart::WEBP_FAST,     "\n";

/* Use a canvas large enough that the four modes produce visibly
 * different byte counts. At tiny sizes (<200x200) lossless overhead
 * dominates and the modes converge. */
$c = (new FastChart\LineChart(800, 600))
    ->setSeries([['data' => range(1, 50)]])
    ->setCategoryLabels(array_map(strval(...), range(1, 50)));

$modes = [
    'DRAWING'  => FastChart\Chart::WEBP_DRAWING,
    'PHOTO'    => FastChart\Chart::WEBP_PHOTO,
    'LOSSLESS' => FastChart\Chart::WEBP_LOSSLESS,
    'FAST'     => FastChart\Chart::WEBP_FAST,
];

$sizes = [];
foreach ($modes as $name => $mode) {
    $c->setWebpMode($mode);
    $w = $c->renderWebp();
    /* Every mode must produce a valid RIFF/WEBP stream:
     *   bytes 0..3  = "RIFF" (0x52494646)
     *   bytes 8..11 = "WEBP" (0x57454250) */
    $is_riff = substr($w, 0, 4) === 'RIFF';
    $is_webp = substr($w, 8, 4) === 'WEBP';
    echo "$name: riff=", $is_riff ? 'yes' : 'no',
        " webp=", $is_webp ? 'yes' : 'no', "\n";
    $sizes[$name] = strlen($w);
}

/* The four modes must produce at least three distinct sizes — if
 * everything is identical, setWebpMode() isn't wired through. */
echo "modes_differ: ", (count(array_unique($sizes)) >= 3 ? "ok" : "fail"), "\n";

/* Setter validation. */
try {
    $c->setWebpMode(7);
    echo "bad_mode_accepted: REGRESSION\n";
} catch (\ValueError $e) {
    $m = $e->getMessage();
    echo "bad_mode_rejected: ",
        (str_contains($m, 'WEBP_DRAWING') && str_contains($m, 'WEBP_LOSSLESS')
            ? "ok" : "wrong-msg"), "\n";
}

/* Mode survives on the same instance across multiple renderWebp
 * calls (sticky, not one-shot). */
$c->setWebpMode(FastChart\Chart::WEBP_LOSSLESS);
$a = strlen($c->renderWebp());
$b = strlen($c->renderWebp());
echo "mode_sticks_across_calls: ", ($a === $b ? "ok" : "fail"), "\n";

/* Symbol family exposes the same setter (QR codes benefit most:
 * lossless preserves bit-exact recovery for machine-readable codes
 * while compressing the flat black/white pattern efficiently). */
$qr = (new FastChart\QrCode())->setData('https://example.com')->setSize(200, 200);
$qr->setWebpMode(FastChart\Chart::WEBP_LOSSLESS);
$qw = $qr->renderWebp();
echo "symbol_setter_works: ",
    (substr($qw, 0, 4) === 'RIFF' && substr($qw, 8, 4) === 'WEBP'
        ? "ok" : "fail"), "\n";

echo "ok\n";
?>
--EXPECT--
WEBP_DRAWING:  0
WEBP_PHOTO:    1
WEBP_LOSSLESS: 2
WEBP_FAST:     3
DRAWING: riff=yes webp=yes
PHOTO: riff=yes webp=yes
LOSSLESS: riff=yes webp=yes
FAST: riff=yes webp=yes
modes_differ: ok
bad_mode_rejected: ok
mode_sticks_across_calls: ok
symbol_setter_works: ok
ok
