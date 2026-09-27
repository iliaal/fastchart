<?php
function fc_resource_child_command(string $code): string
{
    // Preserve the harness's quoting and all extensions, including gd-first runs.
    $args = getenv('TEST_PHP_ARGS') ?: '';
    if ($args === '') {
        $args = '-d ' . escapeshellarg('extension=' . dirname(__DIR__)
            . '/modules/' . (PHP_OS_FAMILY === 'Windows' ? 'php_fastchart.dll' : 'fastchart.so'));
    }
    // Windows escapeshellarg() replaces double quotes inside its argument.
    $script = "eval(base64_decode('" . base64_encode($code) . "'));";
    return (PHP_OS_FAMILY === 'Windows' ? '' : 'exec ')
        . escapeshellarg(PHP_BINARY) . ' ' . $args . ' -r ' . escapeshellarg($script);
}

function fc_resource_child_result($child, array $pipes): string
{
    $status = null;
    for ($i = 0; $i < 20; $i++) {
        usleep(100000);
        $status = proc_get_status($child);
        if (!$status['running']) break;
    }
    $timedOut = $status['running'];
    if ($timedOut) proc_terminate($child);
    foreach ($pipes as $pipe) stream_set_blocking($pipe, false);
    $stdout = stream_get_contents($pipes[1]);
    $stderr = stream_get_contents($pipes[2]);
    foreach ($pipes as $pipe) fclose($pipe);
    $exit = proc_close($child);
    // Before PHP 8.3 the terminal proc_get_status() consumes the exit status.
    if ($exit === -1 && !$timedOut) $exit = $status['exitcode'];
    if ($timedOut) return 'timed-out';
    return $exit === 0 && $stdout === '' && $stderr === '' ? 'completed' : 'failed';
}
