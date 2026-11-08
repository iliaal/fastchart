--TEST--
Atomic abort probe requires the child's failed write, not a tracing failure
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Linux') die('skip Linux strace probe');
if (!function_exists('proc_open')) die('skip proc_open unavailable');
if (!is_readable('/proc/self/maps')) die('skip loaded module path unavailable');
if (!is_writable('/dev/full')) die('skip /dev/full unavailable');
?>
--FILE--
<?php
$source = file_get_contents(__DIR__ . '/438_atomic_abort_interloper.phpt');
$probe = explode('--INI--', explode('--SKIPIF--', $source, 2)[1], 2)[0];
$dir = sys_get_temp_dir() . '/fastchart-abort-probe-' . bin2hex(random_bytes(6));
mkdir($dir, 0700);
$script = $dir . '/probe.php';
file_put_contents($script, $probe);
$strace = $dir . '/strace';
// Exercise the real probe and PHP child without needing ptrace. /dev/full
// supplies a failed write; it does not simulate strace's EIO injection itself.
$fake = <<<'SH'
#!/bin/sh
case "$FC_PROBE_MODE" in
    denied) echo 'ptrace: Operation not permitted' >&2; exit 1 ;;
    launch-error) exit 127 ;;
    other-error) exit 255 ;;
esac
while [ "$#" -gt 0 ] && [ "$1" != '--' ]; do shift; done
[ "$#" -gt 0 ] || exit 2
shift
if [ "$FC_PROBE_MODE" = 'failed-write' ]; then exec "$@" > /dev/full; fi
exec "$@"
SH;
file_put_contents($strace, $fake);
chmod($strace, 0700);
$path = getenv('PATH');
try {
    foreach (['denied', 'launch-error', 'other-error', 'ineffective', 'failed-write', 'missing'] as $mode) {
        putenv('FC_PROBE_MODE=' . $mode);
        putenv('PATH=' . ($mode === 'missing' ? $dir . '/missing' : $dir . ':' . $path));
        $process = proc_open([PHP_BINARY, '-n', $script], [
            ['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w'],
        ], $pipes);
        fclose($pipes[0]);
        $stdout = stream_get_contents($pipes[1]);
        $stderr = stream_get_contents($pipes[2]);
        fclose($pipes[1]);
        fclose($pipes[2]);
        $status = proc_close($process);
        $decision = trim($stdout);
        if (in_array($decision, ['skip strace unavailable', 'skip strace write injection unavailable'], true)) {
            $decision = 'skip';
        }
        echo $mode, ': ', $status, '/', $decision === '' ? 'run' : $decision,
            $stderr === '' ? "\n" : '/unexpected stderr: ' . $stderr;
    }
} finally {
    putenv('PATH=' . $path);
    putenv('FC_PROBE_MODE');
    unlink($strace);
    unlink($script);
    rmdir($dir);
}
?>
--EXPECT--
denied: 0/skip
launch-error: 0/skip
other-error: 0/skip
ineffective: 0/skip
failed-write: 0/run
missing: 0/skip
