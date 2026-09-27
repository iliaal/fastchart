--TEST--
An encoder timeout preserves the prior destination and removes its temporary file
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
if (!function_exists('exec') || getenv('TEST_PHP_ARGS') === false) {
    echo "skip needs exec() and the run-tests TEST_PHP_ARGS\n";
}
?>
--FILE--
<?php
require __DIR__ . '/_encoder_timeout.inc';
fastchartEncoderTimeout('png', true);
?>
--EXPECT--
timeout preserves prior file
