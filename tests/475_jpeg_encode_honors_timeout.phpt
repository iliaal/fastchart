--TEST--
JPEG encoding honors max_execution_time (control for the PNG row poll)
--EXTENSIONS--
fastchart
--FILE--
<?php

/* The JPEG scanline loop is the poll the PNG row loop now matches, so
 * it is the control for tests/474: same armed-budget shape, same
 * expected timeout Error. A bounded legal render still produces a
 * complete JPEG. */

ini_set('memory_limit', '512M');

/* A bailout out of the encoder never reaches the VM interrupt check
 * that raises the timeout Error, so a registered shutdown function is
 * what brings the process back into the VM to report it. */
register_shutdown_function(static function (): void {});

function cpu(): float
{
    $r = getrusage();
    return $r['ru_utime.tv_sec'] + $r['ru_utime.tv_usec'] / 1e6
         + $r['ru_stime.tv_sec'] + $r['ru_stime.tv_usec'] / 1e6;
}

function chart(int $w, int $h): FastChart\LineChart
{
    $points = [];
    for ($i = 0; $i < 200; $i++) { $points[] = (($i * 7919) % 1000) / 1000; }
    return (new FastChart\LineChart($w, $h))
        ->setSeries([['name' => 's', 'data' => $points]]);
}

$jpeg = chart(1024, 1024)->renderJpeg();
var_dump(str_starts_with($jpeg, "\xFF\xD8\xFF") && strlen($jpeg) > 0);

/* Arm two seconds and leave a twentieth of one to the render, so the
 * deadline lands inside it while a pollable stage still has time to
 * respond well inside the hard timeout. */
ini_set('max_execution_time', '2');
$stop = cpu() + 1.95;
while (cpu() < $stop) { /* spend the armed budget */ }

chart(2048, 2048)->renderJpeg();
echo "not reached\n";

?>
--EXPECTF--
bool(true)

Fatal error: Maximum execution time of 2 seconds exceeded in %s on line %d
