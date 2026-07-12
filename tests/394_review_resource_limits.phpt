--TEST--
Review findings: renderToFile rejects non-atomic stream-wrapper destinations
--EXTENSIONS--
fastchart
--FILE--
<?php

final class DestructiveShortWrite
{
    public static string $data = 'GOOD';
    public $context;

    public function stream_open(string $path, string $mode): bool
    {
        self::$data = '';
        return true;
    }

    public function stream_write(string $data): int
    {
        return 0;
    }

    public function stream_close(): void {}
}

stream_wrapper_register('destructive', DestructiveShortWrite::class);
$chart = (new FastChart\LineChart(200, 120))->setSeries([1, 2]);
try {
    $chart->renderToFile('destructive://chart.svg');
    echo "wrapper: NO THROW\n";
} catch (ValueError $e) {
    echo "wrapper: throws\n";
}
echo 'wrapper_preserved: ', DestructiveShortWrite::$data === 'GOOD' ? "yes\n" : "NO\n";

try {
    $chart->renderToFile('file://' . sys_get_temp_dir() . '/fastchart-wrapper.svg');
    echo "file_wrapper: NO THROW\n";
} catch (ValueError $e) {
    echo "file_wrapper: throws\n";
}
@unlink(sys_get_temp_dir() . '/fastchart-wrapper.svg');

?>
--EXPECT--
wrapper: throws
wrapper_preserved: yes
file_wrapper: throws
