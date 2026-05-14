--TEST--
renderGif / renderAvif: removed methods reject via engine undefined-method
--EXTENSIONS--
fastchart
--FILE--
<?php

/* v1.0 dropped GIF and AVIF entirely — including the C-side methods.
 * Calling the method now hits PHP's engine-level "Call to undefined
 * method" path. The test asserts that exact behaviour so a future
 * accidental re-add of renderGif() / renderAvif() returning empty
 * bytes (or any value) would fail this test. */

$c = (new FastChart\LineChart(200, 100))->setSeries([1, 2, 3]);

foreach (['renderGif', 'renderAvif'] as $method) {
    try {
        $c->$method();
        echo "$method: no throw (REGRESSION — method was re-added)\n";
        continue;
    } catch (\Error $e) {
        $msg = $e->getMessage();
        $is_undef = (strpos($msg, 'undefined method') !== false)
                 || (strpos($msg, 'Undefined method') !== false);
        if ($is_undef && strpos($msg, $method) !== false) {
            echo "$method: undefined-method ok\n";
        } else {
            echo "$method: wrong Error: $msg\n";
        }
    }
}

/* renderToFile('.gif' / '.avif') still has an explicit "dropped in
 * v1.0" branch that throws — that path is reachable and should keep
 * working. */
foreach (['gif' => 'GIF', 'avif' => 'AVIF'] as $ext => $label) {
    try {
        $c->renderToFile("/tmp/fc_drop_test_$ext.$ext");
        echo "to_file_$ext: no throw\n";
    } catch (\Error $e) {
        $msg = $e->getMessage();
        if (strpos($msg, "$label output was dropped in v1.0") !== false) {
            echo "to_file_$ext: dropped ok\n";
        } else {
            echo "to_file_$ext: wrong Error: $msg\n";
        }
    }
}

echo "OK\n";
?>
--EXPECT--
renderGif: undefined-method ok
renderAvif: undefined-method ok
to_file_gif: dropped ok
to_file_avif: dropped ok
OK
