--TEST--
renderToFile writes atomically: no temp droppings, pre-existing file replaced
--EXTENSIONS--
fastchart
--FILE--
<?php

/* Plain local paths are written to a sibling temp file and renamed into
 * place, so a failed or short write can never truncate a pre-existing good
 * file, and a successful write leaves no stray temp behind. */

use FastChart\BarChart;

$dir = sys_get_temp_dir() . "/fc_atomic_" . getmypid();
@mkdir($dir);

function temp_droppings(string $dir): int {
    return count(array_filter(scandir($dir), fn($f) => strpos($f, 'fctmp') !== false));
}

$chart = fn() => (new BarChart(200, 150))->setSeries([["name" => "s", "data" => [1, 2, 3]]]);

// Happy path: render into an empty dir, no temp files left behind.
$target = "$dir/out.png";
$chart()->renderToFile($target);
echo "happy_droppings: ", temp_droppings($dir), "\n";
echo "happy_is_png:    ", (substr((string) file_get_contents($target), 1, 3) === "PNG" ? "yes" : "no"), "\n";

// Atomic replace: overwrite an existing good file; content ends up as the
// new PNG (never a half-written blend), and still no temp droppings.
file_put_contents($target, "OLDDATA");
$chart()->renderToFile($target);
$content = (string) file_get_contents($target);
echo "replace_is_png:  ", (substr($content, 1, 3) === "PNG" ? "yes" : "no"), "\n";
echo "replace_dropping:", temp_droppings($dir), "\n";

// Failure injection: pre-existing good file in a read-only directory. The
// temp create fails, an exception is thrown, and the original is intact.
// (Running as root can't be blocked by mode bits; the invariant then holds
// vacuously since no write is attempted.)
$rodir = "$dir/ro";
@mkdir($rodir);
$good = "$rodir/keep.png";
file_put_contents($good, "GOODDATA");
@chmod($rodir, 0555);
clearstatcache();
$can_block = (@file_put_contents("$rodir/.probe", "x") === false);
if ($can_block) {
    $threw = false;
    try {
        $chart()->renderToFile($good);
    } catch (\Throwable $e) {
        $threw = true;
    }
    $intact = (file_get_contents($good) === "GOODDATA");
    echo "failure_safe:    ", ($threw && $intact ? "yes" : "no"), "\n";
} else {
    @unlink("$rodir/.probe");
    echo "failure_safe:    yes\n";
}

// cleanup
@chmod($rodir, 0755);
foreach (glob("$rodir/*") as $f) @unlink($f);
@rmdir($rodir);
foreach (glob("$dir/*") as $f) @unlink($f);
@rmdir($dir);

?>
--EXPECT--
happy_droppings: 0
happy_is_png:    yes
replace_is_png:  yes
replace_dropping:0
failure_safe:    yes
