<?php
/*
 * Performance harness for the three vendored-library optimizations.
 *
 * Scenarios:
 *   label_chart    - text-heavy chart (exercises FT_Load_Glyph + Decompose).
 *                    Targets optimization #1 (glyph outline cache).
 *   qr_v10         - QR code at version ~10 (exercises Reed-Solomon ECC).
 *                    Targets optimization #2 (GF multiply table).
 *   svg_to_png     - Chart::svgToPng with a user-supplied SVG document.
 *                    Targets optimization #3 (single-parse path).
 *   stock_chart    - StockChart with 200 candles + MA + volume pane.
 *                    Sanity check that no regression hits the chart-render
 *                    path that isn't directly targeted.
 *
 * Output: JSON to stdout summarizing min / median / mean / max times in
 * milliseconds for each scenario, plus output byte sizes (regression check
 * — optimization should not change output byte count).
 */

if (PHP_SAPI !== 'cli') {
    fwrite(STDERR, "perf.php must run from CLI\n");
    exit(1);
}
if (!extension_loaded('fastchart')) {
    fwrite(STDERR, "fastchart extension not loaded\n");
    exit(1);
}

$ITERS  = (int)(getenv('FC_BENCH_ITERS') ?: 60);
$WARMUP = (int)(getenv('FC_BENCH_WARMUP') ?: 5);
$LABEL  = getenv('FC_BENCH_LABEL') ?: 'unlabeled';

/* ------------------------------------------------------------------ */

function bench(string $name, int $warmup, int $iters, callable $fn): array {
    /* Warm up: face/library init, allocator priming. */
    $sample_out = null;
    for ($i = 0; $i < $warmup; $i++) {
        $sample_out = $fn();
    }
    $times = [];
    $out_len = is_string($sample_out) ? strlen($sample_out) : 0;
    for ($i = 0; $i < $iters; $i++) {
        $t0 = hrtime(true);
        $out = $fn();
        $t1 = hrtime(true);
        $times[] = ($t1 - $t0) / 1e6;  /* ns -> ms */
        if (is_string($out) && strlen($out) !== $out_len) {
            /* Non-deterministic output size = something is wrong. */
            fwrite(STDERR, "warn: $name output size drifted at iter $i: "
                . "$out_len -> " . strlen($out) . "\n");
            $out_len = strlen($out);
        }
    }
    sort($times);
    $n = count($times);
    return [
        'name'    => $name,
        'iters'   => $n,
        'min'     => $times[0],
        'p50'     => $times[(int)($n * 0.5)],
        'p90'     => $times[(int)($n * 0.9)],
        'max'     => $times[$n - 1],
        'mean'    => array_sum($times) / $n,
        'out_len' => $out_len,
    ];
}

/* ----- Scenario 1: label-heavy chart -------------------------------
 *
 * StockChart with 200 candles. The X-axis emits ~10 date-style labels,
 * Y-axis ~8 price labels, legend has 3 entries, and the title is a
 * full sentence. Lots of repeated digit glyphs.
 */
function scenario_label_chart(): callable {
    mt_srand(424242);  /* deterministic candle data → stable output bytes */
    $rows = [];
    $base_ts = 1700000000;
    $price = 100.0;
    for ($i = 0; $i < 200; $i++) {
        $o = $price;
        $c = $price + sin($i * 0.3) * 4 + (mt_rand(-500, 500) / 250.0);
        $h = max($o, $c) + 1.5;
        $l = min($o, $c) - 1.5;
        $v = 1000 + mt_rand(0, 500);
        $rows[] = [$base_ts + $i * 86400, $o, $h, $l, $c, $v];
        $price = $c;
    }
    return function() use ($rows) {
        $chart = (new FastChart\StockChart())
            ->setSize(1200, 700)
            ->setTitle('Daily prices with 30-day moving average and volume')
            ->setOhlcv($rows)
            ->addMovingAverage(30)
            ->setVolumePane(true);
        return $chart->renderPng();
    };
}

/* ----- Scenario 2: QR code, large + ECC-dominated -------------------
 *
 * v25+ECC-H has roughly 1300 codewords and the largest practical RS
 * encode work — that's where reedSolomonMultiply() time dominates and
 * the GF table win actually shows up over render noise.
 */
function scenario_qr_v10(): callable {
    /* v25-H holds 511 alphanumeric chars. Use 500 to stay safely inside. */
    $payload = str_repeat('ABCD1234EFGH5678ZZZZ', 25);  /* 500 chars */
    return function() use ($payload) {
        return (new FastChart\QrCode())
            ->setData($payload)
            ->setSize(800, 800)
            ->setEcc(FastChart\QrCode::ECC_H)
            ->renderPng();
    };
}

/* ----- Scenario 3: Chart::svgToPng with user-supplied SVG ---------- */
function scenario_svg_to_png(): callable {
    /* ~40 KB SVG with ~500 primitives + glyph-like paths. The
     * baseline path parses this twice (get_intrinsic_dims, then
     * rasterize_svg); the single-pass optimization parses it once.
     * Larger document = parse cost rises above the system noise
     * floor (~0.5-1 ms p50 noise on this ASAN debug build). */
    mt_srand(424242);
    $primitives = [];
    for ($i = 0; $i < 200; $i++) {
        $x = 10 + ($i * 13) % 780;
        $y = 10 + ($i * 19) % 420;
        $w = 5 + mt_rand(0, 30);
        $h = 5 + mt_rand(0, 30);
        $primitives[] = sprintf(
            '<rect x="%d" y="%d" width="%d" height="%d" fill="#%02x%02x%02x"/>',
            $x, $y, $w, $h, $i & 255, ($i * 7) & 255, ($i * 13) & 255);
    }
    /* 100 path elements with multi-segment d strings (mimics text-as-paths). */
    for ($i = 0; $i < 100; $i++) {
        $cx = 50 + ($i * 7) % 700;
        $cy = 50 + ($i * 11) % 350;
        $d = "M{$cx} {$cy}";
        for ($j = 0; $j < 12; $j++) {
            $dx = mt_rand(-5, 5);
            $dy = mt_rand(-5, 5);
            $d .= "l{$dx} {$dy}";
        }
        $d .= 'Z';
        $primitives[] = sprintf('<path d="%s" fill="#%02x%02x%02x"/>',
            $d, ($i * 3) & 255, ($i * 5) & 255, ($i * 11) & 255);
    }
    /* 200 line segments. */
    for ($i = 0; $i < 200; $i++) {
        $primitives[] = sprintf(
            '<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="#%02x%02x%02x" stroke-width="1"/>',
            mt_rand(0, 800), mt_rand(0, 450),
            mt_rand(0, 800), mt_rand(0, 450),
            mt_rand(0, 255), mt_rand(0, 255), mt_rand(0, 255));
    }
    $svg = '<?xml version="1.0" encoding="UTF-8"?>'
        . '<svg xmlns="http://www.w3.org/2000/svg" width="800" height="450" viewBox="0 0 800 450">'
        . '<rect x="0" y="0" width="800" height="450" fill="#fafafa"/>'
        . implode('', $primitives)
        . '</svg>';
    return function() use ($svg) {
        return FastChart\Chart::svgToPng($svg);
    };
}

/* ----- Scenario 4: regression sanity (basic chart) ----------------- */
function scenario_basic_chart(): callable {
    return function() {
        return (new FastChart\LineChart())
            ->setSize(800, 400)
            ->setTitle('regression smoke')
            ->setSeries([3, 8, 4, 11, 6, 9, 13, 7, 10])
            ->renderPng();
    };
}

/* ----- Scenario 6: JPEG encode (exercises opt#8) ------------------- */
function scenario_label_chart_jpeg(): callable {
    mt_srand(424242);
    $rows = [];
    $base_ts = 1700000000;
    $price = 100.0;
    for ($i = 0; $i < 200; $i++) {
        $o = $price;
        $c = $price + sin($i * 0.3) * 4 + (mt_rand(-500, 500) / 250.0);
        $h = max($o, $c) + 1.5;
        $l = min($o, $c) - 1.5;
        $v = 1000 + mt_rand(0, 500);
        $rows[] = [$base_ts + $i * 86400, $o, $h, $l, $c, $v];
        $price = $c;
    }
    return function() use ($rows) {
        return (new FastChart\StockChart())
            ->setSize(1200, 700)
            ->setTitle('Daily prices with 30-day moving average and volume')
            ->setOhlcv($rows)
            ->addMovingAverage(30)
            ->setVolumePane(true)
            ->renderJpeg(88);
    };
}

/* ----- Scenario 5: WebP encode (exercises opt#5) ------------------- */
function scenario_label_chart_webp(): callable {
    mt_srand(424242);
    $rows = [];
    $base_ts = 1700000000;
    $price = 100.0;
    for ($i = 0; $i < 200; $i++) {
        $o = $price;
        $c = $price + sin($i * 0.3) * 4 + (mt_rand(-500, 500) / 250.0);
        $h = max($o, $c) + 1.5;
        $l = min($o, $c) - 1.5;
        $v = 1000 + mt_rand(0, 500);
        $rows[] = [$base_ts + $i * 86400, $o, $h, $l, $c, $v];
        $price = $c;
    }
    return function() use ($rows) {
        return (new FastChart\StockChart())
            ->setSize(1200, 700)
            ->setTitle('Daily prices with 30-day moving average and volume')
            ->setOhlcv($rows)
            ->addMovingAverage(30)
            ->setVolumePane(true)
            ->renderWebp(85);
    };
}

/* ----- Scenario 7: scatter + 200-segment trend fit -----------------
 * Exercises the per-segment vs batched-polyline emit (the trend curve
 * is drawn as 200 sub-segments) and the per-coordinate number format. */
function scenario_scatter_trend(): callable {
    mt_srand(424242);
    $pts = [];
    for ($i = 0; $i < 200; $i++) {
        $pts[] = [$i * 1.3 + mt_rand(-20, 20) / 10.0,
                  5 + $i * 0.4 + sin($i * 0.2) * 6];
    }
    return function() use ($pts) {
        /* renderSvg: the segment-batching + number-format wins live in
         * the SVG-build phase; raster would swamp them with encode time. */
        return (new FastChart\ScatterChart())
            ->setSize(900, 500)
            ->setPoints($pts)
            ->setTrendLine(true)
            ->renderSvg();
    };
}

/* ----- Scenario 8: log-scale line, 1000 points ---------------------
 * Exercises fastchart_y_to_pixel on a log axis 1000x per render — the
 * log10(min)/log10(max) recompute is the target. */
function scenario_log_line(): callable {
    $data = [];
    $v = 1.0;
    for ($i = 0; $i < 1000; $i++) {
        $v *= 1.0 + (sin($i * 0.05) * 0.02 + 0.012);  /* positive, spans decades */
        $data[] = $v;
    }
    return function() use ($data) {
        return (new FastChart\LineChart())
            ->setSize(1000, 500)
            ->setSeries($data)
            ->setYAxisScale(FastChart\Chart::SCALE_LOG)
            ->renderSvg();   /* build-phase: 1000x y_to_pixel on a log axis */
    };
}

/* ----- Scenario 9: coordinate-dense multi-series SVG ---------------
 * Eight series of 500 points each → ~4000 polyline coordinates plus
 * markers, all formatted by fc_svg_fmt_num. renderSvg so the per-
 * coordinate format cost is the whole measurement. */
function scenario_dense_svg(): callable {
    $series = [];
    for ($s = 0; $s < 8; $s++) {
        $d = [];
        for ($i = 0; $i < 500; $i++) {
            $d[] = 50 + sin($i * 0.05 + $s) * 40 + $s * 5 + ($i % 7) * 1.37;
        }
        $series[] = ['data' => $d];
    }
    return function() use ($series) {
        return (new FastChart\LineChart())
            ->setSize(1200, 600)
            ->setSeries($series)
            ->renderSvg();
    };
}

/* ------------------------------------------------------------------ */

$scenarios = [
    'label_chart'   => scenario_label_chart(),
    'qr_v10'        => scenario_qr_v10(),
    'svg_to_png'    => scenario_svg_to_png(),
    'basic_chart'   => scenario_basic_chart(),
    'label_webp'    => scenario_label_chart_webp(),
    'label_jpeg'    => scenario_label_chart_jpeg(),
    'scatter_trend' => scenario_scatter_trend(),
    'log_line'      => scenario_log_line(),
    'dense_svg'     => scenario_dense_svg(),
];

$results = [];
foreach ($scenarios as $name => $fn) {
    $r = bench($name, $WARMUP, $ITERS, $fn);
    $results[$name] = $r;
    fprintf(STDERR,
        "%-14s  min=%6.2f  p50=%6.2f  p90=%6.2f  max=%6.2f  out=%d B\n",
        $name, $r['min'], $r['p50'], $r['p90'], $r['max'], $r['out_len']);
}

echo json_encode([
    'label'     => $LABEL,
    'php'       => PHP_VERSION,
    'iters'     => $ITERS,
    'warmup'    => $WARMUP,
    'extension' => phpversion('fastchart'),
    'results'   => $results,
], JSON_PRETTY_PRINT), "\n";
