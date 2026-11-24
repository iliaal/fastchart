--TEST--
An encoder timeout preserves the prior destination and removes its temporary file
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require_once __DIR__ . '/_codecs.inc';
fc_require_codecs('Png');
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
