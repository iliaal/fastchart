<?php
/* libpng, libjpeg-turbo and libwebp are optional at build time. Call from
 * SKIPIF with the encoders a test needs, e.g. fc_require_codecs('Png'). */
function fc_require_codecs(string ...$codecs): void
{
    static $libs = ['Png' => 'libpng', 'Jpeg' => 'libjpeg-turbo', 'Webp' => 'libwebp'];
    $svg = '<svg xmlns="http://www.w3.org/2000/svg" width="1" height="1"/>';
    foreach ($codecs as $codec) {
        try {
            [FastChart\Chart::class, 'svgTo' . $codec]($svg);
        } catch (\Error $e) {
            if (strpos($e->getMessage(), 'not compiled in') !== false) {
                die("skip {$libs[$codec]} support not compiled in\n");
            }
        }
    }
}
