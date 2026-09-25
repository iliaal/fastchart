--TEST--
Unknown source-image wrapper lookup is silent at render time
--EXTENSIONS--
fastchart
--FILE--
<?php

set_error_handler(static function (int $severity, string $message): never {
    throw new ErrorException($message, 0, $severity);
});
try {
    $svg = (new FastChart\LineChart(120, 80))
        ->setBackgroundImage('missing://image.png')
        ->setSeries([1, 2, 3])
        ->renderSvg();
    echo str_contains($svg, '<svg') ? "rendered\n" : "failed\n";
} catch (Throwable $e) {
    echo "handler-threw\n";
} finally {
    restore_error_handler();
}

?>
--EXPECT--
rendered
