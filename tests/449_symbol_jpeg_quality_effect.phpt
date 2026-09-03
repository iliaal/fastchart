--TEST--
Symbol renderJpeg() quality has a byte-level effect (q=1 vs q=100)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* 130 covers setJpegQuality() range validation; this covers the effect:
 * the quality knob must reach the encoder on the Symbol lane, so q=1
 * and q=100 produce different bytes with the expected size ordering,
 * both framed as valid JPEGs. */
$make = fn() => (new FastChart\QrCode())->setData('QUALITY-449');

$lo = $make()->renderJpeg(1);
$hi = $make()->renderJpeg(100);

echo 'outputs differ: ', $lo !== $hi ? "yes\n" : "NO\n";
echo 'higher quality is larger: ',
	strlen($hi) > strlen($lo) ? "yes\n" : "NO\n";

foreach (['lo' => $lo, 'hi' => $hi] as $label => $bytes) {
	echo $label, ' magic: ',
		str_starts_with($bytes, "\xFF\xD8\xFF")
		&& str_ends_with($bytes, "\xFF\xD9") ? "yes\n" : "NO\n";
}

/* setJpegQuality() sticks on the instance: per-call null reuses it. */
$sym = $make()->setJpegQuality(100);
echo 'sticky quality matches: ',
	$sym->renderJpeg() === $hi ? "yes\n" : "NO\n";

?>
--EXPECT--
outputs differ: yes
higher quality is larger: yes
lo magic: yes
hi magic: yes
sticky quality matches: yes
