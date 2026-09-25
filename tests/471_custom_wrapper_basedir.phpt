--TEST--
Custom non-URL wrappers honor open_basedir narrowed after path assignment
--EXTENSIONS--
fastchart
--FILE--
<?php

final class NarrowedPngStream
{
    public $context;
    private int $position = 0;
    private string $bytes;
    public function __construct()
    {
        $this->bytes = base64_decode(
            'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII='
        );
    }
    public function stream_open(string $path, string $mode): bool { return true; }
    public function stream_read(int $count): string
    {
        $chunk = substr($this->bytes, $this->position, $count);
        $this->position += strlen($chunk);
        return $chunk;
    }
    public function stream_eof(): bool { return $this->position >= strlen($this->bytes); }
    public function stream_stat(): array { return ['mode' => 0100000, 'size' => strlen($this->bytes)]; }
}

stream_wrapper_register('narrowedpng', NarrowedPngStream::class);
$chart = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage('narrowedpng://image.png')
    ->setSeries([1, 2, 3]);
$dir = sys_get_temp_dir() . '/fc_custom_obd_' . getmypid();
@mkdir($dir, 0700);
ini_set('open_basedir', $dir);
$svg = $chart->renderSvg();
echo str_contains($svg, '<image ') ? "custom-wrapper-bypassed\n" : "custom-wrapper-refused\n";
@rmdir($dir);

?>
--EXPECT--
custom-wrapper-refused
