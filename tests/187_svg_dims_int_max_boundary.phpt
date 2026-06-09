--TEST--
Chart::svgToPng(): SVG dimensions at the 2^31 float boundary are rejected
--EXTENSIONS--
fastchart
--FILE--
<?php

/* (float)INT_MAX rounds up to 2^31, so a `>` guard admits
 * width="2147483648" straight into a float-to-int cast that is UB
 * (C11 6.3.1.4). The guard must use `>=`. Functionally identical on
 * x86 either way (the cast happens to produce INT_MIN, caught
 * downstream) — this test exists so the CI UBSan lane locks the
 * boundary in. */

/* plutovg's number parser accumulates in float, so all-digit forms
 * like "2147483648" drift DOWN to 2147483520 (the largest float
 * below 2^31) and take the friendly cap path. The scientific form
 * lands on/above 2^31 and must hit the >= guard. */
$cases = [
    '2.147483648e9',  /* 2^31 — the boundary the guard must reject */
    '1e10',           /* far past the boundary */
];

foreach ($cases as $dim) {
    $svg = '<svg xmlns="http://www.w3.org/2000/svg" width="' . $dim
         . '" height="10"></svg>';
    try {
        FastChart\Chart::svgToPng($svg);
        echo "$dim: no error\n";
    } catch (ValueError $e) {
        $msg = str_contains($e->getMessage(), 'no resolvable intrinsic dimensions')
            ? 'intrinsic-dims' : $e->getMessage();
        echo "$dim: ValueError ($msg)\n";
    }
}

/* Values below 2^31 still convert cleanly; they must keep taking
 * the user-friendly cap path, not the UB-avoidance path. */
foreach (['2147483520', '2147483648'] as $dim) {
    $svg = '<svg xmlns="http://www.w3.org/2000/svg" width="' . $dim
         . '" height="10"></svg>';
    try {
        FastChart\Chart::svgToPng($svg);
        echo "$dim: no error\n";
    } catch (ValueError $e) {
        $msg = str_contains($e->getMessage(), 'exceed cap')
            ? 'cap' : $e->getMessage();
        echo "$dim: ValueError ($msg)\n";
    }
}

?>
--EXPECT--
2.147483648e9: ValueError (intrinsic-dims)
1e10: ValueError (intrinsic-dims)
2147483520: ValueError (cap)
2147483648: ValueError (cap)
