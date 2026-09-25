--TEST--
WebP rendering honors an armed max_execution_time
--EXTENSIONS--
fastchart
--FILE--
<?php

/* libwebp's WebPEncode() is a single call: no progress callback, no way
 * to abort, so a WebP render cannot honor a deadline once the codec is
 * running and the documented guarantee stops at that boundary. The
 * encoder checks the flag before importing the frame and before the
 * encode call; this covers the render end to end under an armed
 * budget, and a bounded legal WebP render stays complete. */

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

/* Bounded legal render: a complete RIFF/WEBP container. */
$webp = chart(1024, 1024)->renderWebp();
var_dump(str_starts_with($webp, 'RIFF') && substr($webp, 8, 4) === 'WEBP');

/* Armed budget: the deadline lands inside a render that needs seconds
 * more, and the render stops with the timeout Error instead of running
 * to completion. */
ini_set('max_execution_time', '2');
$stop = cpu() + 1.95;
while (cpu() < $stop) { /* spend the armed budget */ }

chart(2048, 2048)->renderWebp();
echo "not reached\n";

?>
--EXPECTF--
bool(true)

Fatal error: Maximum execution time of 2 seconds exceeded in %s on line %d
