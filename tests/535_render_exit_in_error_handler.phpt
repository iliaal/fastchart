--TEST--
exit() from an error handler during render stops the script and leaves the chart idle
--EXTENSIONS--
fastchart
--FILE--
<?php
$chart = (new FastChart\LineChart(200, 120))->setSeries([1, 2, 3]);
$chart->setBackgroundImage('fcunknownscheme://bg.png');

register_shutdown_function(function () use ($chart) {
    $chart->setBackgroundImage('')->setTitle('shutdown');
    echo "shutdown render: ",
        str_contains($chart->renderSvg(), '</svg>') ? 'ok' : 'BAD', "\n";
});

set_error_handler(function (int $no, string $str) {
    echo "handler exits\n";
    exit(3);
});
try {
    echo strlen($chart->renderSvg()), " bytes\n";
} catch (Throwable $e) {
    echo "caught ", $e::class, "\n";
}
echo "statement after exit() executed\n";
?>
--EXPECT--
handler exits
shutdown render: ok
