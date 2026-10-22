<?php
/* Some PHP builds end the process on memory_limit exhaustion instead of
 * bailing out: the EOL 8.1 security backport for GHSA-w8hw-w9fx-j79w /
 * GHSA-6wmf-4rv5-p3rp (shipped by setup-php and the shivammathur Homebrew
 * tap as 8.1.34) prints the fatal error and calls _exit(1), so no shutdown
 * function, destructor or RSHUTDOWN runs. Tests that observe the state a
 * bailout leaves behind from a shutdown function cannot run there.
 *
 * Returns false only when a child PHP run with the same binary proves the
 * shutdown function did not run after the limit tripped. Any probe that
 * cannot run or is inconclusive returns true, so the test runs and fails
 * rather than hiding in the skip counter.
 *
 * Usage in --SKIPIF--:
 *
 *     require __DIR__ . '/_bailout_shutdown.inc';
 *     if (!fc_oom_runs_shutdown()) {
 *         die("skip memory_limit exhaustion exits without running shutdown functions\n");
 *     }
 */
function fc_oom_runs_shutdown(): bool
{
    if (!function_exists('exec')) {
        return true;
    }
    $code = 'register_shutdown_function(function () { echo "shutdown-ran\n"; });'
        . ' $s = str_repeat("x", 32 * 1024 * 1024); echo "limit-not-enforced\n";';
    $command = escapeshellarg(PHP_BINARY) . ' -d memory_limit=8M -d display_errors=1'
        . ' -r ' . escapeshellarg($code) . ' 2>&1';
    $output = [];
    $status = 0;
    if (@exec($command, $output, $status) === false) {
        return true;
    }
    $text = implode("\n", $output);
    if (!str_contains($text, 'Allowed memory size of')) {
        return true;
    }
    return str_contains($text, 'shutdown-ran');
}
