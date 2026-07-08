--TEST--
LineChart: STEP_AFTER and STEP_BEFORE both draw staircases
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* The step interpolation modes draw a staircase: every data segment is
 * axis-aligned (horizontal or vertical), never diagonal. STEP_AFTER
 * holds the previous y to the new x then jumps; STEP_BEFORE jumps at
 * the previous x then holds. Linear connects points with diagonals.
 *
 * Regression guard: STEP_BEFORE previously collapsed to a diagonal
 * (identical geometry to LINEAR) because the corner used the current x
 * for both segments. A correct staircase has zero diagonal data
 * segments; the broken one had as many as LINEAR. */

function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

/* Count diagonal coloured data segments (the series stroke is #1F77B4):
 * a segment with both x and y changing. Axis/grid lines use other
 * colours, so keying on the series colour isolates the data path.
 * Series runs are emitted as polyline elements now; keep support for
 * the old line-element shape so the assertion describes geometry, not
 * the specific SVG primitive. */
function diagonals(string $svg): int {
    $n = 0;
    preg_match_all('/<line\b[^>]*stroke="#1F77B4"[^>]*>/', $svg, $m);
    foreach ($m[0] as $line) {
        if (preg_match('/x1="(-?\d+)" y1="(-?\d+)" x2="(-?\d+)" y2="(-?\d+)"/', $line, $c)) {
            if ($c[1] !== $c[3] && $c[2] !== $c[4]) $n++;
        }
    }
    preg_match_all('/<polyline\b[^>]*stroke="#1F77B4"[^>]*>/', $svg, $pm);
    foreach ($pm[0] as $poly) {
        if (!preg_match('/points="([^"]+)"/', $poly, $p)) continue;
        preg_match_all('/(-?\d+),(-?\d+)/', $p[1], $pts, PREG_SET_ORDER);
        for ($i = 1; $i < count($pts); $i++) {
            if ($pts[$i - 1][1] !== $pts[$i][1] &&
                $pts[$i - 1][2] !== $pts[$i][2]) {
                $n++;
            }
        }
    }
    return $n;
}

$data = [['data' => [3, 7, 4, 8, 5]]];

$lin   = (new FastChart\LineChart(360, 220))->setSeries($data)
             ->setLineInterpolation(FastChart\Chart::INTERP_LINEAR)->renderSvg();
$stepA = (new FastChart\LineChart(360, 220))->setSeries($data)
             ->setLineInterpolation(FastChart\Chart::INTERP_STEP_AFTER)->renderSvg();
$stepB = (new FastChart\LineChart(360, 220))->setSeries($data)
             ->setLineInterpolation(FastChart\Chart::INTERP_STEP_BEFORE)->renderSvg();

echo "all_valid: ", (valid($lin) && valid($stepA) && valid($stepB)) ? "yes" : "no", "\n";
echo "linear_has_diagonals: ", (diagonals($lin) > 0) ? "yes" : "no", "\n";
echo "stepA_no_diagonals: ", (diagonals($stepA) === 0) ? "yes" : "no", "\n";
echo "stepB_no_diagonals: ", (diagonals($stepB) === 0) ? "yes" : "no", "\n";
echo "stepA_ne_stepB: ", ($stepA !== $stepB) ? "yes" : "no", "\n";
echo "stepB_ne_linear: ", ($stepB !== $lin) ? "yes" : "no", "\n";

echo "ok\n";
?>
--EXPECT--
all_valid: yes
linear_has_diagonals: yes
stepA_no_diagonals: yes
stepB_no_diagonals: yes
stepA_ne_stepB: yes
stepB_ne_linear: yes
ok
