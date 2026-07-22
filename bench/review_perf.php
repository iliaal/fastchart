<?php

declare(strict_types=1);

if (PHP_SAPI !== 'cli') {
    fwrite(STDERR, "review_perf.php must run from CLI\n");
    exit(1);
}
if (!extension_loaded('fastchart')) {
    fwrite(STDERR, "fastchart extension not loaded\n");
    exit(1);
}

function cli_error(string $message): void
{
    fwrite(STDERR, "review_perf.php: {$message}\n");
    exit(2);
}

function option_value(array $argv, int &$index, string $argument): string
{
    $equals = strpos($argument, '=');
    if ($equals !== false) {
        $value = substr($argument, $equals + 1);
        if ($value === '') {
            cli_error(substr($argument, 0, $equals) . ' requires a value');
        }
        return $value;
    }

    $index++;
    if (!isset($argv[$index]) || str_starts_with($argv[$index], '--')) {
        cli_error("{$argument} requires a value");
    }
    return $argv[$index];
}

function parse_options(array $argv): array
{
    $options = [
        'scenarios' => [],
        'iterations' => null,
        'warmup' => null,
        'raster_size' => 4096,
        'label' => 'unlabeled',
        'pretty' => false,
        'list' => false,
        'help' => false,
    ];

    for ($i = 1, $n = count($argv); $i < $n; $i++) {
        $argument = $argv[$i];
        if ($argument === '--help') {
            $options['help'] = true;
        } elseif ($argument === '--list') {
            $options['list'] = true;
        } elseif ($argument === '--pretty') {
            $options['pretty'] = true;
        } elseif ($argument === '--scenario'
            || str_starts_with($argument, '--scenario=')) {
            $value = option_value($argv, $i, $argument);
            foreach (explode(',', $value) as $scenario) {
                $scenario = trim($scenario);
                if ($scenario === '') {
                    cli_error('--scenario contains an empty name');
                }
                $options['scenarios'][] = $scenario;
            }
        } elseif ($argument === '--iterations'
            || str_starts_with($argument, '--iterations=')) {
            $options['iterations'] = option_value($argv, $i, $argument);
        } elseif ($argument === '--warmup'
            || str_starts_with($argument, '--warmup=')) {
            $options['warmup'] = option_value($argv, $i, $argument);
        } elseif ($argument === '--raster-size'
            || str_starts_with($argument, '--raster-size=')) {
            $options['raster_size'] = option_value($argv, $i, $argument);
        } elseif ($argument === '--label'
            || str_starts_with($argument, '--label=')) {
            $options['label'] = option_value($argv, $i, $argument);
        } else {
            cli_error("unknown argument {$argument}");
        }
    }

    $integer_options = [
        'iterations' => [1, 100000],
        'warmup' => [0, 100000],
        'raster_size' => [64, 8192],
    ];
    foreach ($integer_options as $key => [$minimum, $maximum]) {
        if ($options[$key] === null) {
            continue;
        }
        $raw = (string)$options[$key];
        if ($raw === '' || strspn($raw, '0123456789') !== strlen($raw)) {
            cli_error("--" . str_replace('_', '-', $key)
                . ' must contain only decimal digits');
        }
        $value = (int)$raw;
        if ($value < $minimum || $value > $maximum) {
            cli_error("--" . str_replace('_', '-', $key)
                . " must be in [{$minimum}, {$maximum}]");
        }
        $options[$key] = $value;
    }

    return $options;
}

function usage(): string
{
    return <<<'TXT'
Usage: php -d extension=modules/fastchart.so bench/review_perf.php [options]

  --scenario=NAME       Scenario or group; repeat or comma-separate.
                        Default: smoke. Use --list to inspect names.
  --iterations=N        Override per-scenario safe defaults. Use 80 or
                        more for release measurements.
  --warmup=N            Override per-scenario warmup iterations.
  --raster-size=N       Square raster dimension for raster_large_png.
                        Default: 4096; accepted range: 64..8192.
  --label=TEXT          Label stored in the JSON result.
  --pretty              Pretty-print JSON.
  --list                Emit scenario/group metadata as JSON and exit.
  --help                Show this help.

Examples:
  php -d extension=modules/fastchart.so bench/review_perf.php --pretty
  php -d extension=modules/fastchart.so bench/review_perf.php \
      --scenario=stock --iterations=80 --warmup=5 --label=release
  php -d extension=modules/fastchart.so bench/review_perf.php \
      --scenario=raster_large_png --iterations=3 --raster-size=4096
TXT;
}

function percentile(array $sorted, float $quantile): float
{
    $count = count($sorted);
    if ($count === 1) {
        return $sorted[0];
    }
    $position = ($count - 1) * $quantile;
    $lower = (int)floor($position);
    $upper = (int)ceil($position);
    if ($lower === $upper) {
        return $sorted[$lower];
    }
    $fraction = $position - $lower;
    return $sorted[$lower]
        + ($sorted[$upper] - $sorted[$lower]) * $fraction;
}

function make_stock_rows(int $count): array
{
    $rows = [];
    $timestamp = 1700000000;
    for ($i = 0; $i < $count; $i++) {
        $open = 1000000000.0 + sin($i * 0.071) * 0.25
            + (($i % 17) - 8) * 0.001;
        $close = $open + cos($i * 0.113) * 0.02;
        $rows[] = [
            $timestamp + $i * 86400,
            $open,
            max($open, $close) + 0.05,
            min($open, $close) - 0.05,
            $close,
            1000 + (($i * 37) % 500),
        ];
    }
    return $rows;
}

function make_stock_outlier_rows(int $count): array
{
    $rows = [];
    for ($i = 0; $i < $count; $i++) {
        $close = $i === 0 ? 1.0e308 : 100.0 + sin($i * 0.1);
        $rows[] = [1700000000 + $i, $close, $close, $close, $close, 1000];
    }
    return $rows;
}

function stock_variance_runner(array $rows, int $period): Closure
{
    return static function (bool $capture) use ($rows, $period): array {
        $chart = (new FastChart\StockChart(1200, 700))
            ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
            ->setOhlcv($rows);

        $start = hrtime(true);
        $chart->addBollingerBands($period)->addStdDev($period);
        $duration = hrtime(true) - $start;

        return [
            'duration_ns' => $duration,
            'output' => $capture ? $chart->renderSvg() : null,
            'counters' => [
                'candles' => count($rows),
                'period' => $period,
                'setter_calls' => 2,
                'setters' => ['addBollingerBands', 'addStdDev'],
                'timed_scope' => 'indicator setters only; setOhlcv and SVG excluded',
            ],
        ];
    };
}

function stock_extrema_runner(array $rows, array $periods): Closure
{
    return static function (bool $capture) use ($rows, $periods): array {
        $chart = (new FastChart\StockChart(1200, 700))
            ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
            ->setOhlcv($rows);

        $start = hrtime(true);
        $chart
            ->addStochastic($periods['stochastic'], 3)
            ->addWilliamsR($periods['williams_r'])
            ->addAroon($periods['aroon']);
        $duration = hrtime(true) - $start;

        return [
            'duration_ns' => $duration,
            'output' => $capture ? $chart->renderSvg() : null,
            'counters' => [
                'candles' => count($rows),
                'periods' => $periods,
                'stochastic_smoothing' => 3,
                'setter_calls' => 3,
                'setters' => ['addStochastic', 'addWilliamsR', 'addAroon'],
                'timed_scope' => 'indicator setters only; setOhlcv and SVG excluded',
            ],
        ];
    };
}

function scatter_linear_axis_runner(): Closure
{
    $points = [];
    for ($i = 0; $i < 4096; $i++) {
        $points[] = [1000000.0 + ($i * 0.25), 50.0 + sin($i / 17.0)];
    }
    $chart = (new FastChart\ScatterChart(1200, 700))
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setPoints($points);

    return static function (bool $capture) use ($chart): array {
        $start = hrtime(true);
        $output = $chart->renderSvg();
        $duration = hrtime(true) - $start;

        return [
            'duration_ns' => $duration,
            'output' => $output,
            'counters' => [
                'points' => 4096,
                'axis_span_kind' => 'ordinary finite positive',
                'timed_scope' => 'renderSvg',
            ],
        ];
    };
}

function make_words(int $count): array
{
    $words = [];
    for ($i = 0; $i < $count; $i++) {
        $words[] = [
            'text' => 'word-' . str_pad((string)$i, 3, '0', STR_PAD_LEFT),
            'weight' => 1 + (($i * 37) % 100),
        ];
    }
    return $words;
}

function wordcloud_runner(int $width, int $height, string $font): Closure
{
    $words = make_words(256);
    $chart = (new FastChart\WordCloud($width, $height))
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setFontPath($font)
        ->setWords($words);

    return static function (bool $capture) use (
        $chart, $width, $height, $font
    ): array {
        $start = hrtime(true);
        $output = $chart->renderSvg();
        $duration = hrtime(true) - $start;

        return [
            'duration_ns' => $duration,
            'output' => $output,
            'counters' => [
                'words' => 256,
                'width' => $width,
                'height' => $height,
                'font' => basename($font),
                'text_elements' => substr_count($output, '<text'),
                'timed_scope' => 'renderSvg',
            ],
        ];
    };
}

function icon_chart(string $icon): FastChart\LineChart
{
    $values = [];
    for ($i = 0; $i < 32; $i++) {
        $values[] = 50.0 + sin($i * 0.35) * 20.0;
    }
    $chart = (new FastChart\LineChart(1000, 500))
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setSeries($values);
    foreach ($values as $index => $value) {
        $chart->addIconAt($index, $value, $icon, 24, 24);
    }
    return $chart;
}

function icon_svg_counters(string $output, string $icon): array
{
    return [
        'placements' => 32,
        'source' => 'docs/examples/11_plot_bands_annotations.png',
        'source_bytes' => filesize($icon),
        'source_sha256' => hash_file('sha256', $icon),
        'data_uri_count' => substr_count($output, 'data:image/png;base64,'),
        'image_elements' => substr_count($output, '<image'),
        'use_elements' => substr_count($output, '<use'),
    ];
}

function icon_svg_runner(string $icon): Closure
{
    $chart = icon_chart($icon);
    return static function (bool $capture) use ($chart, $icon): array {
        $start = hrtime(true);
        $output = $chart->renderSvg();
        $duration = hrtime(true) - $start;
        $counters = icon_svg_counters($output, $icon);
        $counters['timed_scope'] = 'renderSvg';
        return [
            'duration_ns' => $duration,
            'output' => $output,
            'counters' => $counters,
        ];
    };
}

function icon_png_runner(string $icon): Closure
{
    $chart = icon_chart($icon);
    $characterization = $chart->renderSvg();
    $counters = icon_svg_counters($characterization, $icon);
    $counters['characterization_svg_bytes'] = strlen($characterization);
    $counters['timed_scope'] = 'renderPng';

    return static function (bool $capture) use ($chart, $counters): array {
        $start = hrtime(true);
        $output = $chart->renderPng();
        $duration = hrtime(true) - $start;
        return [
            'duration_ns' => $duration,
            'output' => $output,
            'counters' => $counters,
        ];
    };
}

function write_solid_png(string $path, int $red, int $green, int $blue): void
{
    $color = sprintf('#%02x%02x%02x', $red, $green, $blue);
    $svg = '<svg xmlns="http://www.w3.org/2000/svg" '
        . 'width="1152" height="1152" viewBox="0 0 1152 1152">'
        . '<rect width="1152" height="1152" fill="' . $color . '"/>'
        . '</svg>';
    $png = FastChart\Chart::svgToPng($svg);
    file_put_contents($path, $png);
}

function icon_pressure_runner(): Closure
{
    $dir = sys_get_temp_dir() . '/fastchart-bench-icons-'
        . bin2hex(random_bytes(6));
    mkdir($dir, 0700);
    $paths = [];
    for ($i = 0; $i < 13; $i++) {
        $path = $dir . '/icon-' . $i . '.png';
        write_solid_png($path,
            ($i * 37) & 0xff, ($i * 67) & 0xff, ($i * 97) & 0xff);
        $paths[] = $path;
    }
    register_shutdown_function(static function () use ($paths, $dir): void {
        foreach ($paths as $path) @unlink($path);
        @rmdir($dir);
    });

    $values = array_fill(0, 32, 50.0);
    $chart = (new FastChart\LineChart(960, 360))
        ->setPlotRect(20, 20, 939, 339)
        ->setYAxisRange(0.0, 100.0)
        ->setSeries($values);
    for ($i = 0; $i < 12; $i++) {
        $chart->addIconAt((float)$i, 50.0, $paths[$i], 24, 24);
    }
    $chart->addIconAt(12.0, 50.0, $paths[12], 24, 24);
    for ($i = 0; $i < 12; $i++) {
        $chart->addIconAt((float)($i + 13), 50.0, $paths[$i], 24, 24);
    }
    for ($i = 25; $i < 32; $i++) {
        $chart->addIconAt((float)$i, 50.0, $paths[12], 24, 24);
    }

    return static function (bool $capture) use ($chart): array {
        $start = hrtime(true);
        $output = $chart->renderPng();
        $duration = hrtime(true) - $start;
        return [
            'duration_ns' => $duration,
            'output' => $capture ? $output : null,
            'counters' => [
                'distinct_images' => 13,
                'placements' => 32,
                'decoded_working_set_bytes' => 13 * 1152 * 1152 * 4,
                'cache_limit_bytes' => 64 * 1024 * 1024,
                'hot_tail_placements' => 8,
                'timed_scope' => 'renderPng under decoded-image cache pressure',
            ],
        ];
    };
}

function gradient_runner(): Closure
{
    $values = [];
    for ($i = 0; $i < 1000; $i++) {
        $values[] = 1 + (($i * 37) % 100);
    }
    $chart = (new FastChart\BarChart(12000, 600))
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setSeries($values)
        ->setGradientFill(0x2266AA, 0x88CCEE,
            FastChart\Chart::GRADIENT_VERTICAL);

    return static function (bool $capture) use ($chart): array {
        $start = hrtime(true);
        $output = $chart->renderSvg();
        $duration = hrtime(true) - $start;
        return [
            'duration_ns' => $duration,
            'output' => $output,
            'counters' => [
                'bars' => 1000,
                'gradient_definitions' => substr_count($output, '<linearGradient'),
                'gradient_references' => substr_count($output, 'url(#'),
                'timed_scope' => 'renderSvg',
            ],
        ];
    };
}

function make_network_fixture(): array
{
    $nodes = [];
    $links = [];
    for ($i = 0; $i < 512; $i++) {
        $nodes[] = [];
        $links[] = ['from' => $i, 'to' => ($i + 1) % 512, 'value' => 1];
        $links[] = ['from' => $i, 'to' => ($i + 17) % 512, 'value' => 2];
    }
    return [$nodes, $links];
}

function network_chart(array $nodes, array $links): FastChart\NetworkChart
{
    return (new FastChart\NetworkChart(1000, 800))
        ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
        ->setSeed(424242)
        ->setIterations(300)
        ->setNodes($nodes)
        ->setLinks($links);
}

function network_cold_runner(): Closure
{
    [$nodes, $links] = make_network_fixture();
    return static function (bool $capture) use ($nodes, $links): array {
        $chart = network_chart($nodes, $links);
        $start = hrtime(true);
        $output = $chart->renderSvg();
        $duration = hrtime(true) - $start;
        return [
            'duration_ns' => $duration,
            'output' => $output,
            'counters' => [
                'nodes' => 512,
                'links' => 1024,
                'requested_iterations' => 300,
                'cache_state' => 'cold',
                'timed_scope' => 'first renderSvg on a fresh chart',
            ],
        ];
    };
}

function network_hot_runner(): Closure
{
    [$nodes, $links] = make_network_fixture();
    $chart = network_chart($nodes, $links);
    $prime = $chart->renderSvg();
    $prime_hash = hash('sha256', $prime);

    return static function (bool $capture) use ($chart, $prime_hash): array {
        $start = hrtime(true);
        $output = $chart->renderSvg();
        $duration = hrtime(true) - $start;
        return [
            'duration_ns' => $duration,
            'output' => $output,
            'counters' => [
                'nodes' => 512,
                'links' => 1024,
                'requested_iterations' => 300,
                'cache_state' => 'hot',
                'prime_sha256' => $prime_hash,
                'timed_scope' => 'renderSvg after one untimed prime render',
            ],
        ];
    };
}

function max_rss(): array
{
    $usage = getrusage();
    $value = isset($usage['ru_maxrss']) ? (int)$usage['ru_maxrss'] : null;
    return [
        'value' => $value,
        'unit' => PHP_OS_FAMILY === 'Darwin' ? 'bytes' : 'KiB',
    ];
}

function raster_runner(int $size): Closure
{
    $values = [];
    for ($i = 0; $i < 512; $i++) {
        $values[] = 100 + sin($i * 0.07) * 25 + cos($i * 0.013) * 10;
    }
    $chart = (new FastChart\LineChart($size, $size))->setSeries($values);

    return static function (bool $capture) use ($chart, $size): array {
        $before = max_rss();
        $start = hrtime(true);
        $output = $chart->renderPng();
        $duration = hrtime(true) - $start;
        $after = max_rss();
        $delta = $before['value'] !== null && $after['value'] !== null
            ? max(0, $after['value'] - $before['value']) : null;

        return [
            'duration_ns' => $duration,
            'output' => $output,
            'counters' => [
                'width' => $size,
                'height' => $size,
                'pixels' => $size * $size,
                'series_points' => 512,
                'max_render_pixels_ini' => (int)ini_get('fastchart.max_render_pixels'),
                'ru_maxrss_before' => $before['value'],
                'ru_maxrss_after' => $after['value'],
                'ru_maxrss_delta' => $delta,
                'ru_maxrss_unit' => $after['unit'],
                'timed_scope' => 'renderPng',
            ],
        ];
    };
}

function file_sink_runner(string $format): Closure
{
    $values = [];
    for ($i = 0; $i < 1000; $i++) {
        $values[] = 100 + sin($i * 0.07) * 25 + cos($i * 0.013) * 10;
    }
    $chart = (new FastChart\LineChart(1600, 900))->setSeries($values);
    $path = sys_get_temp_dir() . '/fastchart-file-sink-' .
        bin2hex(random_bytes(12)) . ".{$format}";

    return static function (bool $capture) use (
        $chart, $format, $path
    ): array {
        try {
            $start = hrtime(true);
            $chart->renderToFile($path);
            $duration = hrtime(true) - $start;
            $output = $capture ? file_get_contents($path) : null;
            if ($capture && !is_string($output)) {
                throw new RuntimeException('unable to read rendered file');
            }
        } finally {
            @unlink($path);
        }

        return [
            'duration_ns' => $duration,
            'output' => $output,
            'counters' => [
                'format' => $format,
                'series_points' => 1000,
                'width' => 1600,
                'height' => 900,
                'timed_scope' => 'renderToFile including atomic replacement',
            ],
        ];
    };
}

function discover_fonts(): array
{
    $patterns = [
        '/usr/share/fonts/truetype/*/*.ttf',
        '/usr/share/fonts/truetype/*/*.otf',
        '/usr/share/fonts/*/*.ttf',
        '/usr/share/fonts/TTF/*.ttf',
        '/Library/Fonts/*.ttf',
        '/Library/Fonts/*.ttc',
        '/System/Library/Fonts/*.ttf',
        '/System/Library/Fonts/*.ttc',
        'C:/Windows/Fonts/*.ttf',
    ];
    $fonts = [];
    foreach ($patterns as $pattern) {
        foreach (glob($pattern) ?: [] as $path) {
            $real = realpath($path);
            if ($real === false || isset($fonts[$real]) || !is_readable($real)) {
                continue;
            }
            $bytes = filesize($real);
            if ($bytes !== false && $bytes > 0 && $bytes <= 4 * 1024 * 1024) {
                $fonts[$real] = $bytes;
            }
        }
    }
    arsort($fonts, SORT_NUMERIC);
    return array_slice($fonts, 0, 4, true);
}

function font_cache_runner(array $fonts): Closure
{
    $charts = [];
    foreach (array_keys($fonts) as $index => $font) {
        $charts[] = (new FastChart\LineChart(480, 240))
            ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
            ->setFontPath($font)
            ->setTitle("font cache {$index}: 0123456789")
            ->setCategoryLabels(['alpha', 'beta', 'gamma'])
            ->setSeries([1, 3, 2]);
    }

    $prime = '';
    foreach ($charts as $chart) {
        $prime .= $chart->renderSvg();
    }
    $prime_hash = hash('sha256', $prime);
    $font_names = array_map('basename', array_keys($fonts));

    return static function (bool $capture) use (
        $charts, $fonts, $font_names, $prime_hash
    ): array {
        $start = hrtime(true);
        $output = '';
        foreach ($charts as $chart) {
            $output .= $chart->renderSvg();
        }
        $duration = hrtime(true) - $start;
        return [
            'duration_ns' => $duration,
            'output' => $output,
            'counters' => [
                'font_count' => count($fonts),
                'font_names' => $font_names,
                'aggregate_font_bytes' => array_sum($fonts),
                'aggregate_cache_budget_bytes' => 16 * 1024 * 1024,
                'cache_slots_targeted' => 4,
                'prime_sha256' => $prime_hash,
                'timed_scope' => 'four renderSvg calls after one untimed cache-warming pass',
            ],
        ];
    };
}

function run_scenario(string $name, array $definition, int $iterations,
    int $warmup): array
{
    if (isset($definition['skip_reason'])) {
        return [
            'name' => $name,
            'status' => 'skipped',
            'reason' => $definition['skip_reason'],
            'iterations' => 0,
            'warmup_iterations' => 0,
        ];
    }

    try {
        $runner = $definition['factory']();
        for ($i = 0; $i < $warmup; $i++) {
            $runner(false);
        }

        $times = [];
        $output_bytes = [];
        $output_hashes = [];
        $counters = null;
        for ($i = 0; $i < $iterations; $i++) {
            $capture = $i === 0 || $i === $iterations - 1;
            $sample = $runner($capture);
            if (!isset($sample['duration_ns']) || $sample['duration_ns'] < 0) {
                throw new RuntimeException('scenario returned an invalid duration');
            }
            $times[] = $sample['duration_ns'] / 1000000.0;
            if ($counters === null) {
                $counters = $sample['counters'] ?? [];
            }
            if (($sample['output'] ?? null) !== null) {
                if (!is_string($sample['output'])) {
                    throw new RuntimeException('scenario output is not a string');
                }
                $output_bytes[] = strlen($sample['output']);
                $output_hashes[] = hash('sha256', $sample['output']);
            }
        }
        if ($output_hashes === []) {
            throw new RuntimeException('scenario did not capture output');
        }

        $sorted = $times;
        sort($sorted, SORT_NUMERIC);
        $unique_hashes = array_values(array_unique($output_hashes));
        $unique_bytes = array_values(array_unique($output_bytes));
        $stable = count($unique_hashes) === 1 && count($unique_bytes) === 1;

        return [
            'name' => $name,
            'status' => $stable ? 'ok' : 'unstable-output',
            'description' => $definition['description'],
            'iterations' => $iterations,
            'warmup_iterations' => $warmup,
            'p50_ms' => round(percentile($sorted, 0.50), 6),
            'p90_ms' => round(percentile($sorted, 0.90), 6),
            'min_ms' => round($sorted[0], 6),
            'max_ms' => round($sorted[count($sorted) - 1], 6),
            'mean_ms' => round(array_sum($times) / count($times), 6),
            'samples_ms' => array_map(
                static fn(float $time): float => round($time, 6), $times),
            'output_bytes' => $output_bytes[0],
            'output_sha256' => $output_hashes[0],
            'output_capture_count' => count($output_hashes),
            'output_hash_stable' => count($unique_hashes) === 1,
            'output_bytes_stable' => count($unique_bytes) === 1,
            'distinct_output_hashes' => count($unique_hashes),
            'counters' => $counters,
        ];
    } catch (Throwable $exception) {
        return [
            'name' => $name,
            'status' => 'error',
            'description' => $definition['description'],
            'iterations' => 0,
            'warmup_iterations' => $warmup,
            'error_class' => $exception::class,
            'error' => $exception->getMessage(),
        ];
    }
}

$options = parse_options($argv);
if ($options['help']) {
    echo usage(), "\n";
    exit(0);
}

$stock_rows = make_stock_rows(4096);
$stock_outlier_rows = make_stock_outlier_rows(4096);
$icon = realpath(__DIR__ . '/../docs/examples/11_plot_bands_annotations.png');
if ($icon === false || !is_readable($icon)) {
    cli_error('tracked icon fixture is missing or unreadable');
}
$fonts = discover_fonts();
$wordcloud_font = null;
foreach (array_keys($fonts) as $font_path) {
    if (basename($font_path) === 'DejaVuSans.ttf') {
        $wordcloud_font = $font_path;
        break;
    }
}
$wordcloud_font ??= array_key_first($fonts);

$definitions = [
    'stock_variance_default' => [
        'description' => '4096-candle Bollinger and StdDev setters, period 20',
        'default_iterations' => 5,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => stock_variance_runner($stock_rows, 20),
    ],
    'stock_variance_worst' => [
        'description' => '4096-candle Bollinger and StdDev setters, period 2048',
        'default_iterations' => 5,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => stock_variance_runner($stock_rows, 2048),
    ],
    'stock_variance_outlier_default' => [
        'description' => '4096 candles with one extreme outlier, period 20',
        'default_iterations' => 20,
        'default_warmup' => 3,
        'factory' => static fn(): Closure =>
            stock_variance_runner($stock_outlier_rows, 20),
    ],
    'stock_variance_outlier_worst' => [
        'description' => '4096 candles with one extreme outlier, period 2048',
        'default_iterations' => 10,
        'default_warmup' => 2,
        'factory' => static fn(): Closure =>
            stock_variance_runner($stock_outlier_rows, 2048),
    ],
    'stock_extrema_default' => [
        'description' => '4096-candle Stochastic/Williams/Aroon default windows',
        'default_iterations' => 5,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => stock_extrema_runner($stock_rows, [
            'stochastic' => 14,
            'williams_r' => 14,
            'aroon' => 25,
        ]),
    ],
    'stock_extrema_worst' => [
        'description' => '4096-candle Stochastic/Williams/Aroon period 2048',
        'default_iterations' => 5,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => stock_extrema_runner($stock_rows, [
            'stochastic' => 2048,
            'williams_r' => 2048,
            'aroon' => 2048,
        ]),
    ],
    'scatter_linear_axis' => [
        'description' => '4096-point Scatter render on ordinary linear axes',
        'default_iterations' => 20,
        'default_warmup' => 3,
        'factory' => static fn(): Closure => scatter_linear_axis_runner(),
    ],
    'wordcloud_300x200' => array_merge([
        'description' => '256-word WordCloud render at 300x200',
        'default_iterations' => 3,
        'default_warmup' => 1,
        'factory' => static fn(): Closure =>
            wordcloud_runner(300, 200, $wordcloud_font),
    ], $wordcloud_font !== null ? [] : [
        'skip_reason' => 'no readable system font found',
    ]),
    'wordcloud_1000x800' => array_merge([
        'description' => '256-word WordCloud render at 1000x800',
        'default_iterations' => 2,
        'default_warmup' => 1,
        'factory' => static fn(): Closure =>
            wordcloud_runner(1000, 800, $wordcloud_font),
    ], $wordcloud_font !== null ? [] : [
        'skip_reason' => 'no readable system font found',
    ]),
    'icons_svg' => [
        'description' => '32 repeated addIconAt placements rendered to SVG',
        'default_iterations' => 3,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => icon_svg_runner($icon),
    ],
    'icons_png' => [
        'description' => '32 repeated addIconAt placements rendered to PNG',
        'default_iterations' => 1,
        'default_warmup' => 0,
        'factory' => static fn(): Closure => icon_png_runner($icon),
    ],
    'icons_png_pressure' => [
        'description' => '13 decoded 1152px images with a repeated hot tail',
        'default_iterations' => 5,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => icon_pressure_runner(),
    ],
    'gradients_svg' => [
        'description' => '1000 bars sharing one gradient rendered to SVG',
        'default_iterations' => 3,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => gradient_runner(),
    ],
    'network_cold' => [
        'description' => '512-node Network first render on a fresh chart',
        'default_iterations' => 2,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => network_cold_runner(),
    ],
    'network_hot' => [
        'description' => '512-node Network render after layout cache priming',
        'default_iterations' => 5,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => network_hot_runner(),
    ],
    'raster_large_png' => [
        'description' => 'large square PNG render with native RSS high-water counters',
        'default_iterations' => 1,
        'default_warmup' => 0,
        'factory' => static fn(): Closure => raster_runner($options['raster_size']),
    ],
    'file_sink_png' => [
        'description' => '1600x900 PNG rendered through atomic renderToFile',
        'default_iterations' => 3,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => file_sink_runner('png'),
    ],
    'file_sink_jpeg' => [
        'description' => '1600x900 JPEG rendered through atomic renderToFile',
        'default_iterations' => 3,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => file_sink_runner('jpg'),
    ],
    'file_sink_webp' => [
        'description' => '1600x900 WebP rendered through atomic renderToFile',
        'default_iterations' => 3,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => file_sink_runner('webp'),
    ],
    'font_cache_warm' => array_merge([
        'description' => 'four-font aggregate face-cache hot render pass',
        'default_iterations' => 3,
        'default_warmup' => 1,
        'factory' => static fn(): Closure => font_cache_runner($fonts),
    ], count($fonts) === 4 ? [] : [
        'skip_reason' => 'fewer than four readable system fonts found',
    ]),
];

$groups = [
    'smoke' => [
        'stock_variance_default',
        'stock_extrema_default',
        'scatter_linear_axis',
        'wordcloud_300x200',
        'icons_svg',
        'gradients_svg',
        'network_hot',
    ],
    'stock' => [
        'stock_variance_default',
        'stock_variance_worst',
        'stock_variance_outlier_default',
        'stock_variance_outlier_worst',
        'stock_extrema_default',
        'stock_extrema_worst',
    ],
    'axes' => ['scatter_linear_axis'],
    'wordcloud' => ['wordcloud_300x200', 'wordcloud_1000x800'],
    'icons' => ['icons_svg', 'icons_png', 'icons_png_pressure'],
    'network' => ['network_cold', 'network_hot'],
    'resources' => [
        'icons_svg', 'icons_png', 'icons_png_pressure', 'gradients_svg',
    ],
    'memory' => ['raster_large_png', 'font_cache_warm'],
    'file_sinks' => ['file_sink_png', 'file_sink_jpeg', 'file_sink_webp'],
    'all' => array_keys($definitions),
];

if ($options['list']) {
    $listing = [];
    foreach ($definitions as $name => $definition) {
        $listing[$name] = [
            'description' => $definition['description'],
            'default_iterations' => $definition['default_iterations'],
            'default_warmup' => $definition['default_warmup'],
            'available' => !isset($definition['skip_reason']),
            'skip_reason' => $definition['skip_reason'] ?? null,
        ];
    }
    echo json_encode([
        'scenarios' => $listing,
        'groups' => $groups,
    ], JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES | JSON_THROW_ON_ERROR), "\n";
    exit(0);
}

$requested = $options['scenarios'] ?: ['smoke'];
$selected = [];
foreach ($requested as $name) {
    if (isset($groups[$name])) {
        foreach ($groups[$name] as $member) {
            $selected[$member] = true;
        }
    } elseif (isset($definitions[$name])) {
        $selected[$name] = true;
    } else {
        cli_error("unknown scenario or group {$name}; use --list");
    }
}

$results = [];
$exit_code = 0;
foreach (array_keys($selected) as $name) {
    $definition = $definitions[$name];
    $iterations = $options['iterations']
        ?? $definition['default_iterations'];
    $warmup = $options['warmup'] ?? $definition['default_warmup'];
    $result = run_scenario($name, $definition, $iterations, $warmup);
    if ($result['status'] !== 'ok' && $result['status'] !== 'skipped') {
        $exit_code = 1;
    }
    $results[$name] = $result;
}

$document = [
    'schema_version' => 1,
    'label' => $options['label'],
    'generated_at_utc' => gmdate('c'),
    'php_version' => PHP_VERSION,
    'php_sapi' => PHP_SAPI,
    'fastchart_version' => FastChart\Chart::version(),
    'platform' => PHP_OS . ' ' . php_uname('m'),
    'config' => [
        'requested' => $requested,
        'selected' => array_keys($selected),
        'iterations_override' => $options['iterations'],
        'warmup_override' => $options['warmup'],
        'raster_size' => $options['raster_size'],
    ],
    'results' => $results,
];

$flags = JSON_UNESCAPED_SLASHES | JSON_PRESERVE_ZERO_FRACTION
    | JSON_THROW_ON_ERROR;
if ($options['pretty']) {
    $flags |= JSON_PRETTY_PRINT;
}
echo json_encode($document, $flags), "\n";
exit($exit_code);
