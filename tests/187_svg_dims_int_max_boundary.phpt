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

/* The scientific form lands on/above 2^31 and must hit the >= guard
 * (the intrinsic-dims path) on every target.
 *
 * The all-digit form is architecture-dependent. plutosvg's number
 * parser accumulates in float via `10.f * integer + digit` (a fused-
 * multiply-add candidate), and FP_CONTRACT is on by default:
 *   - no FMA (x86-64 baseline v1/v2): two rounding steps drift the
 *     value DOWN to 2147483520 (< 2^31), so it casts cleanly and the
 *     downstream per-axis check rejects it with the "exceed cap" msg.
 *   - FMA (aarch64 always; x86-64-v3 baseline e.g. EL-10): the fused
 *     op is exactly rounded to 2^31, trips the >= guard, and is
 *     rejected with the "intrinsic-dims" msg.
 * Both reject — which is the contract — so accept either message. */
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

/* All-digit forms at the boundary must be REJECTED, but which of the
 * two rejection paths fires is FP-contraction-dependent (see above).
 * Assert rejection with one of the two known messages, not a specific
 * one — but still reject an unexpected message (tighter than %s). */
foreach (['2147483520', '2147483648'] as $dim) {
    $svg = '<svg xmlns="http://www.w3.org/2000/svg" width="' . $dim
         . '" height="10"></svg>';
    try {
        FastChart\Chart::svgToPng($svg);
        echo "$dim: no error\n";
    } catch (ValueError $e) {
        $m = $e->getMessage();
        $known = str_contains($m, 'exceed cap')
              || str_contains($m, 'no resolvable intrinsic dimensions');
        echo "$dim: ValueError (", $known ? 'cap-or-intrinsic-dims' : $m, ")\n";
    }
}

?>
--EXPECT--
2.147483648e9: ValueError (intrinsic-dims)
1e10: ValueError (intrinsic-dims)
2147483520: ValueError (cap-or-intrinsic-dims)
2147483648: ValueError (cap-or-intrinsic-dims)
