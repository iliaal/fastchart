--TEST--
Source-image loader retains custom stream-wrapper compatibility
--EXTENSIONS--
fastchart
--FILE--
<?php

final class ResourcePngStream
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

    public function stream_stat(): array
    {
        return ['mode' => 0100000, 'size' => strlen($this->bytes)];
    }
}

stream_wrapper_register('resourcepng', ResourcePngStream::class);
$svg = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage('resourcepng://image.png')
    ->setSeries([1, 2, 3])
    ->renderSvg();
echo str_contains($svg, '<image ') ? "wrapper-image-ok\n" : "wrapper-image-missing\n";
$file = tempnam(sys_get_temp_dir(), 'fc_file_uri_');
file_put_contents($file, base64_decode(
    'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII='
));
$fileSvg = (new FastChart\LineChart(120, 80))
    ->setBackgroundImage('file://' . $file)
    ->setSeries([1, 2, 3])
    ->renderSvg();
@unlink($file);
echo str_contains($fileSvg, '<image ') ? "file-uri-image-ok\n" : "file-uri-image-missing\n";

?>
--EXPECT--
wrapper-image-ok
file-uri-image-ok
