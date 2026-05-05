--TEST--
Null values in series produce line gaps; strict mode rejects bad types
--EXTENSIONS--
fastchart
--FILE--
<?php

// Series with two gaps (null values). Renders successfully.
$im = imagecreatetruecolor(800, 400);
$out = (new FastChart\LineChart)
    ->setSize(800, 400)
    ->setSeries([10, 20, null, 40, 50, null, 70])
    ->draw($im);
var_dump($out instanceof \GdImage);

// Encode and confirm valid PNG.
ob_start(); imagepng($im); $png = ob_get_clean();
echo "png_sig: ", substr(bin2hex($png), 0, 8) === '89504e47' ? "ok" : "bad", "\n";

// Strict mode tolerates null but rejects other non-numeric types.
$im2 = imagecreatetruecolor(800, 400);
$out = (new FastChart\LineChart)
    ->setSize(800, 400)
    ->setStrict(true)
    ->setSeries([10, null, 30, null, 50])
    ->draw($im2);
echo "strict_with_nulls: ", $out instanceof \GdImage ? "ok" : "bad", "\n";

// Strict + non-numeric (not null) -> TypeError
try {
    (new FastChart\LineChart)
        ->setStrict(true)
        ->setSeries([10, "twenty", 30])
        ->draw(imagecreatetruecolor(400, 200));
    echo "strict_string: no throw\n";
} catch (\TypeError $e) {
    echo "strict_string: TypeError ok\n";
}

// AreaChart honors null sentinel too.
$im3 = imagecreatetruecolor(800, 400);
$out = (new FastChart\AreaChart)
    ->setSize(800, 400)
    ->setSeries([1, 5, null, 8, 10])
    ->draw($im3);
echo "area_with_null: ", $out instanceof \GdImage ? "ok" : "bad", "\n";
?>
--EXPECT--
bool(true)
png_sig: ok
strict_with_nulls: ok
strict_string: TypeError ok
area_with_null: ok
