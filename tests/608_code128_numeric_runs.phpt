--TEST--
Code128 preserves exact SVG output across numeric-run lengths and subset switches
--EXTENSIONS--
fastchart
--FILE--
<?php
// Golden digests were recorded before batching subset-C digit pairs. Cover
// every run length that fits the 80-byte payload cap, including odd tails,
// A/B-to-C switches, C-to-A/B switches, SHIFT and repeated numeric runs.
$contexts = [
    'numeric' => ['', ''],
    'B-prefix' => ['abc', ''],
    'A-prefix' => ["\t\t", ''],
    'B-suffix' => ['', 'xyz'],
    'A-suffix' => ['', "\t\t"],
    'B-surrounded' => ['abc', 'xyz'],
    'A-surrounded' => ["\t\t", "\t\t"],
    'repeated-runs' => ['12abc', 'xyz345678'],
    'shift' => ["abc\t", "\txyz"],
];
$digits = str_repeat('0123456789', 8);
foreach ($contexts as $name => [$prefix, $suffix]) {
    $hash = hash_init('sha256');
    $count = 80 - strlen($prefix) - strlen($suffix);
    for ($length = 1; $length <= $count; $length++) {
        $svg = (new FastChart\Code128())
            ->setData($prefix . substr($digits, 0, $length) . $suffix)
            ->setSize(1200, 80)->setShowText(false)->renderSvg();
        hash_update($hash, $svg);
    }
    echo "$name ($count): ", hash_final($hash), "\n";
}
?>
--EXPECT--
numeric (80): 3810521913dcd6009dc0ec01528b68ec44e760fa1c7646316daf1ea48020e8b2
B-prefix (77): b5bcb03983671e8bcde623d18491edc3453d8681a2614471b47bbf97f58d76e3
A-prefix (78): 86716abf8dd49493eea2249e4140d886d82114a8d46fa1a05ae0ed0fff33ea76
B-suffix (77): fd41fa982ebd0162b076d9fc3c5cac91a7a054409cce95b8534f40f53d00ace6
A-suffix (78): a5624943d7b53fb94627bb11c2a04b1420ddeeed91153af9068014e2cc42029a
B-surrounded (74): 2588391d58a8d0deea8ce2e8209b26a8dec1c364a941370fa77e84634ddc046b
A-surrounded (76): a92518f2957a743085d1c593ba7abb2495cb3608efd5501611994738491656f4
repeated-runs (66): c1d00eb09ffbda50a3a445345a14511c73f099f43287b3c5b50f7bf5db5b83c6
shift (72): 7d70f1992ccf9088c4b3518edee1166c963ddc1ead43359feb7844b58fb44e2d
