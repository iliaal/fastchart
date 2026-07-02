--TEST--
drawSvgFragment($idPrefix) namespaces gradient/clip ids for stitched documents
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Review fix: fragments each restart gradient/clip ids at fcg1/fcc1,
 * so stitching two gradient-bearing fragments into one host document
 * made chart 2's url(#fcg1) resolve to chart 1's gradient. */

use FastChart\BarChart;

function frag(?string $prefix): string {
    $c = (new BarChart(300, 200))
        ->setSeries([3, 7, 5])
        ->setGradientFill(0x2266AA, 0x88CCEE);
    return $prefix === null ? $c->drawSvgFragment() : $c->drawSvgFragment($prefix);
}

/* Default behavior unchanged: bare fcg ids, byte-stable across calls. */
$plain = frag(null);
echo "bare_ids: ", (str_contains($plain, 'id="fcg1"') ? 'yes' : 'NO'), "\n";
echo "deterministic: ", (frag(null) === $plain ? 'yes' : 'NO'), "\n";

/* Prefixed: both the def id and every url() reference carry it. */
$a = frag('a');
echo "prefixed_def: ", (str_contains($a, 'id="afcg1"') ? 'yes' : 'NO'), "\n";
echo "prefixed_ref: ", (str_contains($a, 'url(#afcg1)') ? 'yes' : 'NO'), "\n";
echo "no_bare_refs: ", (!preg_match('/url\(#fcg\d/', $a) ? 'yes' : 'NO'), "\n";

/* Two prefixed fragments share no ids. */
$b = frag('b');
preg_match_all('/id="([^"]+)"/', $a . $b, $m);
$ids = $m[1];
echo "stitched_ids_unique: ", (count($ids) === count(array_unique($ids)) ? 'yes' : 'NO'), "\n";

/* Validation. */
foreach (['starts-with-digit' => '1abc', 'too-long' => str_repeat('a', 17),
          'bad-char' => 'a.b'] as $what => $bad) {
    try {
        frag($bad);
        echo "$what: NO THROW\n";
    } catch (ValueError $e) {
        echo "$what: throws\n";
    }
}

?>
--EXPECT--
bare_ids: yes
deterministic: yes
prefixed_def: yes
prefixed_ref: yes
no_bare_refs: yes
stitched_ids_unique: yes
starts-with-digit: throws
too-long: throws
bad-char: throws
