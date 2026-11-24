--TEST--
PNG encoding stops before committing a file when its deadline expires
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require_once __DIR__ . '/_codecs.inc';
fc_require_codecs('Png');
if (!function_exists('exec') || getenv('TEST_PHP_ARGS') === false) {
    echo 'skip needs exec() and run-tests TEST_PHP_ARGS';
}
?>
--FILE--
<?php
require __DIR__ . '/_encoder_timeout.inc';
fastchartEncoderTimeout('png');
?>
--EXPECT--
timeout before file commit
