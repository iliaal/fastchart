--TEST--
QrCode SVG output is stable across versions and ECC levels at full capacity
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Payloads fill each (version, ECC) slot exactly, so the encoder cannot
 * boost to a higher ECC level; each level's Reed-Solomon block layout runs
 * at every sampled version. Hashes pin the upstream nayuki output. */
$caps = [
    1  => [41, 34, 27, 17],
    2  => [77, 63, 48, 34],
    7  => [370, 293, 207, 154],
    10 => [652, 513, 364, 288],
    25 => [3057, 2395, 1718, 1286],
    40 => [7089, 5596, 3993, 3057],
];
$digits = str_repeat('31415926535897932384626433832795', 222);
foreach ($caps as $v => $row) {
    foreach (['L', 'M', 'Q', 'H'] as $ecc => $name) {
        $qr = fn(int $n) => (new FastChart\QrCode())
            ->setSize(400, 400)
            ->setData(substr($digits, 0, $n))
            ->setEcc($ecc)->setMinVersion($v)->setMaxVersion($v);
        try {
            $qr($row[$ecc] + 1)->renderSvg();
            $full = 'BUG: over capacity accepted';
        } catch (ValueError $e) {
            $full = 'full';
        }
        echo "v$v-$name $full ", md5($qr($row[$ecc])->renderSvg()), "\n";
    }
}
?>
--EXPECT--
v1-L full 45eaf14b8199a9e240ed894c703683aa
v1-M full cced8ba59e1d7651df0bae7a336bd1b7
v1-Q full 44ab01c3ce135fc63c018dcbe8efd3aa
v1-H full be3d75bb5aee9d5ae72252b4a67b394e
v2-L full a469abfbfd1b5cccb70e8a11fe06e16c
v2-M full 59de6a9d6030a20eb20f8686a94e6c03
v2-Q full a48fb33bd83686151a3784957a5fcccf
v2-H full 3fe16574cf76c9adc047e90f0d4a8b45
v7-L full 1f8879042f5066f6222e9e1282034a52
v7-M full f1e2fe7aac128b0345d99f198e60f5bc
v7-Q full 87cda5ee0bbb7aa35b456fe875f1055d
v7-H full 1ed14a9176b1083458da5e2edfbcf1a7
v10-L full 246c86e6da5c6d3a9781d7aa13209e82
v10-M full a5c67b3d0f126af61f4b2aa9b21fc512
v10-Q full 546b6682a028e7dd0de66f9d3bb2ffc1
v10-H full 2847152a14cbd06367af3c6de63dd553
v25-L full f923bbcbac865b19ff8028ac7221173d
v25-M full 5171cda5186a0f79317c937683953ee2
v25-Q full 3628a3c7216464fc1c110842bb77eb3f
v25-H full 460efccf17594afed019f52947a312b3
v40-L full 8206aee460a99fb3f24505be383dbd25
v40-M full c2721e265ba3dd08a23ff1bdf36a9b20
v40-Q full 6dac5bd2341f359d9395ed4dd5e37349
v40-H full 30fc1db5159cfa2427625b81b4998e31
