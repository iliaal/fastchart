--TEST--
WebP encoding cancels before committing a file when its deadline expires
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (!function_exists('exec') || getenv('TEST_PHP_ARGS') === false) {
    echo 'skip needs exec() and run-tests TEST_PHP_ARGS';
}
?>
--FILE--
<?php
require __DIR__ . '/_encoder_timeout.inc';
fastchartEncoderTimeout('webp');
?>
--EXPECT--
timeout before file commit
