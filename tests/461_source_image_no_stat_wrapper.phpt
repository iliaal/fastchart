--TEST--
Source-image custom wrapper without stream_stat remains silent and usable
--EXTENSIONS--
fastchart
--FILE--
<?php

final class ResourcePngNoStatStream
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

    public function stream_open(string $path, string $mode): bool
    {
        return true;
    }

    public function stream_read(int $count): string
    {
        $chunk = substr($this->bytes, $this->position, $count);
        $this->position += strlen($chunk);
        return $chunk;
    }

    public function stream_eof(): bool
    {
        return $this->position >= strlen($this->bytes);
    }
}

stream_wrapper_register('resourcepngnostat', ResourcePngNoStatStream::class);
$svg = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage('resourcepngnostat://image.png')
    ->setSeries([1, 2, 3])
    ->renderSvg();
echo str_contains($svg, '<image ') ? "no-stat-wrapper-ok\n" : "no-stat-wrapper-missing\n";

?>
--EXPECT--
no-stat-wrapper-ok
