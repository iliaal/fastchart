<?php
if (!is_executable('/bin/bash') || !is_executable('/usr/bin/readelf')) {
    throw new Exception('bash and readelf are required');
}
// Execute the committed workflow steps, rather than a second copy of the gates.
$root = dirname(__DIR__);
$dir = sys_get_temp_dir() . '/fastchart-gates-' . bin2hex(random_bytes(8));
mkdir($dir);
mkdir($dir . '/bin');
mkdir($dir . '/modules');
file_put_contents($dir . '/modules/fastchart.so', 'not an ELF object');
file_put_contents($dir . '/bin/php', "#!/bin/bash\nprintf reached > \"\$RUNNER_TEMP/runtime-reached\"\n");
chmod($dir . '/bin/php', 0700);
file_put_contents($dir . '/bin/readelf', "#!/bin/bash\ncat \"\$RUNNER_TEMP/payload\"\nexit \"\$PROBE_RC\"\n");
chmod($dir . '/bin/readelf', 0700);
function step(string $file, string $name): string {
    $text = file_get_contents($file);
    $start = strpos($text, '      - name: ' . $name . "\n");
    if ($start === false) throw new Exception('missing step: ' . $name);
    $text = substr($text, $start);
    if (!preg_match('/^        run: \|\n((?:          [^\n]*\n|\n)+)/m', $text, $m)) {
        throw new Exception('missing run block: ' . $name);
    }
    return preg_replace('/^          /m', '', $m[1]);
}
$steps = [
    'optional' => step($root . '/.github/workflows/tests.yml', 'Verify SVG-only runtime and exact codec errors'),
    'static' => step($root . '/.github/workflows/release-linux.yml', 'Verify codec libs are static (Linux)'),
];
$cases = [
    'clean' => [" 0 (NEEDED) Shared library: [libc.so.6]\n", 0, true, true],
    'empty successful' => ['', 0, true, true],
    'png' => [" 0 (NEEDED) Shared library: [libpng16.so.16]\n", 0, false, false],
    'jpeg uppercase' => [" 0 (NEEDED) Shared library: [LIBJPEG.so.62]\n", 0, false, false],
    'webp' => [" 0 (NEEDED) Shared library: [libwebp.so.7]\n", 0, false, false],
    'freetype' => [" 0 (NEEDED) Shared library: [libfreetype.so.6]\n", 0, true, false],
    'non-needed' => [" 0 (SONAME) Shared library: [libpng.so]\n", 0, true, true],
    'failed empty' => ['', 7, false, false],
    'failed clean' => [" 0 (NEEDED) Shared library: [libc.so.6]\n", 7, false, false],
    'failed forbidden' => [" 0 (NEEDED) Shared library: [libpng.so]\n", 7, false, false],
    'early forbidden' => [" 0 (NEEDED) Shared library: [libpng.so]\n" . str_repeat(" 0 (NEEDED) Shared library: [libc.so.6]\n", 10000), 0, false, false],
    'late forbidden' => [str_repeat(" 0 (NEEDED) Shared library: [libc.so.6]\n", 10000) . " 0 (NEEDED) Shared library: [libpng.so]\n", 0, false, false],
    'real malformed object' => ['', 0, false, false],
];
try {
    foreach ($steps as $lane => $shell) {
        $passed = 0;
        foreach ($cases as $name => [$payload, $rc, $optional, $static]) {
            file_put_contents($dir . '/payload', $payload);
            @unlink($dir . '/runtime-reached');
            $env = getenv();
            $env['RUNNER_TEMP'] = $dir;
            $env['PROBE_RC'] = (string)$rc;
            $env['PATH'] = $dir . '/bin:' . $env['PATH'];
            // Use the real inspector for the malformed-object case.
            $run = $name === 'real malformed object'
                ? str_replace('readelf -d', '/usr/bin/readelf -d', $shell) : $shell;
            $proc = proc_open(['/bin/bash', '--noprofile', '--norc', '-e', '-o', 'pipefail', '-c', $run],
                [0 => ['file', '/dev/null', 'r'], 1 => ['file', $dir . '/stdout', 'w'],
                 2 => ['file', $dir . '/stderr', 'w']], $pipes, $dir, $env);
            if (!is_resource($proc)) throw new Exception('cannot start bash');
            $status = proc_close($proc);
            $expected = $lane === 'optional' ? $optional : $static;
            $ok = ($status === 0) === $expected;
            if ($lane === 'optional') $ok = $ok && file_exists($dir . '/runtime-reached') === $expected;
            if ($rc !== 0) $ok = $ok && $status === $rc;
            if ($name === 'real malformed object') {
                $ok = $ok && str_contains(file_get_contents($dir . '/stderr'), 'readelf:');
            }
            if (!$ok) throw new Exception("$lane $name: FAIL (status=$status)");
            else $passed++;
        }
        echo "$lane: $passed/", count($cases), " cases passed\n";
    }
} finally {
    foreach (glob($dir . '/bin/*') as $file) unlink($file);
    unlink($dir . '/modules/fastchart.so');
    rmdir($dir . '/bin');
    rmdir($dir . '/modules');
    foreach (glob($dir . '/*') as $file) unlink($file);
    rmdir($dir);
}
?>
