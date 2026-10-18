--TEST--
exit() from a stream wrapper during a render-time image load stops the script
--EXTENSIONS--
fastchart
--FILE--
<?php
final class PolicyStream
{
    public static string $mode = 'ok';
    public $context;
    private int $position = 0;
    private string $bytes = '';

    public function stream_open(string $path, string $mode): bool
    {
        $this->bytes = base64_decode(
            'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII='
        );
        switch (self::$mode) {
            case 'throw-open':
                throw new RuntimeException('open denied');
            case 'refuse':
                return false;
            case 'exit':
                echo "calling exit()\n";
                exit(7);
        }
        return true;
    }

    public function stream_read(int $count): string
    {
        if (self::$mode === 'throw-read') {
            throw new RuntimeException('read failed');
        }
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
stream_wrapper_register('fcpolicy', PolicyStream::class);

function chart(string $bg): FastChart\LineChart
{
    return (new FastChart\LineChart(200, 120))->setSeries([1, 2, 3])
        ->setBackgroundImage($bg);
}

function report(string $label, Closure $render): void
{
    try {
        $out = $render();
        echo "$label: returned ", get_debug_type($out),
            str_contains((string)$out, '<image') ? ' with image' : '', "\n";
    } catch (Throwable $e) {
        echo "$label: ", $e::class, ': ', $e->getMessage(), "\n";
    }
}

PolicyStream::$mode = 'ok';
report('ok svg', fn() => chart('fcpolicy://bg.png')->renderSvg());

/* Resource loads stay best-effort: a wrapper that fails or throws, or an
 * error handler that throws on the suppressed lookup warning, leaves the
 * chart rendering without the image. Only exit() escapes the load. */
PolicyStream::$mode = 'refuse';
report('refuse svg', fn() => chart('fcpolicy://bg.png')->renderSvg());

PolicyStream::$mode = 'throw-open';
report('throw-open svg', fn() => chart('fcpolicy://bg.png')->renderSvg());
report('throw-open png', fn() => chart('fcpolicy://bg.png')->renderPng());
$file = sys_get_temp_dir() . '/fc_policy_' . getmypid() . '.png';
report('throw-open file', fn() => chart('fcpolicy://bg.png')->renderToFile($file));
@unlink($file);

PolicyStream::$mode = 'throw-read';
report('throw-read svg', fn() => chart('fcpolicy://bg.png')->renderSvg());

set_error_handler(function (int $no, string $str) {
    throw new ErrorException($str, 0, $no);
});
report('handler setter', fn() => strlen(chart('fcunknownscheme://bg.png')->renderSvg()));
restore_error_handler();
$c = chart('fcunknownscheme://bg.png');
set_error_handler(function (int $no, string $str) {
    throw new ErrorException('handler: ' . substr($str, 0, 39), 0, $no);
});
report('handler render', fn() => $c->renderSvg());
restore_error_handler();

PolicyStream::$mode = 'exit';
$c = (new FastChart\LineChart(200, 120))->setSeries([1, 2, 3])
    ->addIconAt(1, 1, 'fcpolicy://icon.png');
try {
    echo strlen($c->renderSvg()), " bytes\n";
} catch (Throwable $e) {
    echo "caught ", $e::class, "\n";
}
echo "statement after exit() executed\n";
?>
--EXPECT--
ok svg: returned string with image
refuse svg: returned string
throw-open svg: returned string
throw-open png: returned string
throw-open file: returned int
throw-read svg: returned string
handler setter: returned int
handler render: returned string
calling exit()
