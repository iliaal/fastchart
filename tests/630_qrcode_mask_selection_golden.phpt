--TEST--
QrCode mask selection matches upstream at every version and ECC level
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Mask selection scores all eight candidate masks. Each line pins the SVG of
 * every ECC level for numeric, alphanumeric, and byte payloads at one
 * minimum version, so a changed penalty term that flips any chosen mask
 * changes a hash. Hashes pin the upstream nayuki output. */
$alnum = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:';
for ($v = 1; $v <= 40; $v++) {
    $svgs = '';
    foreach ([0, 1, 2, 3] as $ecc) {
        $seed = hash('sha256', "v$v-e$ecc");
        $numeric = '';
        $text = '';
        $bytes = '';
        for ($i = 0; strlen($bytes) < 4 * $v; $i++) {
            foreach (str_split(hash('sha256', $seed . $i, true)) as $c) {
                $bytes .= chr(ord($c) % 255 + 1);  /* setData() rejects NUL */
                $numeric .= (string)(ord($c) % 10);
                $text .= $alnum[ord($c) % 45];
            }
        }
        foreach ([$numeric, $text, $bytes] as $data) {
            $svgs .= (new FastChart\QrCode())
                ->setSize(200, 200)
                ->setData(substr($data, 0, 4 * $v))
                ->setEcc($ecc)->setMinVersion($v)
                ->renderSvg();
        }
    }
    echo "v$v ", md5($svgs), "\n";
}
?>
--EXPECT--
v1 b6390cf3e2adeed148ec2bb0a705ee79
v2 35557776751d9acedf141f6db3e0dae6
v3 fc941f6fcf21b1d8ec183bf75767f940
v4 42441460905f6d2685bec662420758fd
v5 e995d3eca94328c8c6294ea16cf8b55e
v6 659fcedb38e4ade82f764dad47fb734f
v7 293a031c2cbd8deee58df51a19816f9b
v8 2e5a3aa6d9df353f077125b590113b48
v9 8c815087f6d94fe153dd29031d29dcf0
v10 cd76c89e2c57041632844ff0f5d7a0be
v11 7546dbbfae8818e9c4155dd2e87c8275
v12 a262cc3af8257df1ba031896e63e4680
v13 d9a885ba15efb43eda2bf513f78b9e2e
v14 ffe8269af5fab19a74a191a1abad8941
v15 4766c748977b3709f412bc08935ccc83
v16 242963404d745d81649d0e8032dabd72
v17 92d19fa8a47c714a90f548118f109ef2
v18 9a56e43410b3146010949e6d7ae725be
v19 43874817f36d0df655695622b0edd92c
v20 e6fcd87f3f83c8343bd75abd19a6b66e
v21 cb575b26cfaf3130ace1151814fed420
v22 9544087814bcec686723ac2db5a52e11
v23 f3936e2c5f9ae2ecd7d8f1d62d88b289
v24 610abcca7661b337d9e89924c0b44444
v25 722807c5c6c117dbead1cd197b239b3b
v26 685045d4e66defc665b4a6964cfe3b39
v27 acab43924e5680546a1b89c08fc263a2
v28 ed1421732ed084ae6f8440fdd36e3a78
v29 859b30a195ef1c3072489774c5ec928a
v30 e60ab94b8b201917354e25782c93eb03
v31 bf02b2842f12905c1bda9c69135b2b60
v32 eca4ede6db11824242c405db483c501a
v33 1a6ef903378818f59fb94944aa0e66ac
v34 2acc4717a9821c935f80bef9f3185f10
v35 cc734c3adefac2240831f5c44e62f5ec
v36 5971c669a4a6419550adc1db1a92be04
v37 ec33d8553b1262ab6a1fa0c415e5a2ae
v38 09b7a4093b7f83194b82ad95fe330d74
v39 a03a772ebb0660271fcb786fb4d3aedb
v40 d886930690a3a624b301716fabb608da
