# fastchart

[![Tests](https://github.com/iliaal/fastchart/actions/workflows/tests.yml/badge.svg)](https://github.com/iliaal/fastchart/actions/workflows/tests.yml)
[![Version](https://img.shields.io/github/v/tag/iliaal/fastchart?label=version&sort=semver)](https://github.com/iliaal/fastchart/releases)
[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD--3--Clause-green.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![Follow @iliaa](https://img.shields.io/badge/Follow-@iliaa-000000?style=flat&logo=x&logoColor=white)](https://x.com/intent/follow?screen_name=iliaa)

Native C PHP extension. 19 chart types behind a modern OO API with
fluent setters and `final` classes. Line, area, bar, scatter, bubble,
pie, radar, polar, surface, contour, gauge, gantt, box-plot, treemap,
funnel, waterfall, heatmap, linear meter, plus a deep `StockChart`
(seven candle styles, SMA / EMA / WMA overlays, volume + indicator
panes).

Two render paths. `renderToFile()` / `renderPng()` / `renderJpeg()` /
`renderWebp()` / `renderAvif()` cover the common case: declare a
chart, get a file or bytes back. `draw($canvas)` is the other path.
Hand fastchart a `\GdImage` you own and it returns the same canvas,
so you can composite several charts onto one image, stamp arbitrary
[ext/gd](https://www.php.net/manual/en/book.image.php) draw calls
over the result, or drop the rendered chart into a larger image
pipeline (PDF page, sprite sheet, dashboard tile).

![fastchart: 19 chart types in one PHP extension](images/fastchart-hero.jpg)

## Status

Working. 19 chart types, 105 public methods, 97 phpt tests. The OO
surface is stable for the v0.1 line. See
[`CHANGELOG.md`](CHANGELOG.md) for what's shipped.

## Install

Build from source against the PHP install you want to extend:

```sh
phpize
./configure --enable-fastchart
make -j
make test
```

Strict-warnings dev build (recommended for contributors):

```sh
./configure --enable-fastchart --enable-fastchart-dev
```

Runtime check:

```sh
php -d extension=./modules/fastchart.so \
  -r 'echo FastChart\Chart::version(), PHP_EOL;'
```

## Requirements

- PHP 8.3 or later (NTS or ZTS).
- `ext/gd` enabled. fastchart pulls the underlying `gdImagePtr` out
  of caller-supplied `\GdImage` zvals via the one PHPAPI symbol ext/gd
  exports.
- libgd development headers at build time (`libgd-dev` on Debian /
  Ubuntu, `gd-devel` on RHEL / Fedora).
- FreeType (already a libgd dependency on every Linux distribution),
  used for TrueType / OpenType label rendering.

## Quick start

The shortest path is the `renderToFile()` helper, which picks the
encoder from the file extension:

```php
(new FastChart\LineChart(640, 320))
    ->setTitle('Daily active users')
    ->setSeries([['data' => [820, 940, 870, 1020, 1180, 1250, 1340]]])
    ->setCategoryLabels(['Mon','Tue','Wed','Thu','Fri','Sat','Sun'])
    ->renderToFile('/tmp/dau.png');
```

`renderPng()`, `renderJpeg()`, `renderWebp()`, and `renderAvif()` return
the encoded bytes if you need them in memory.

For pixel-level control or compositing several charts on one image,
hand fastchart a `\GdImage` you own. `draw()` returns the same canvas
back so call chains keep working:

```php
$canvas = imagecreatetruecolor(1200, 600);

(new FastChart\StockChart())
    ->setSize(1200, 600)
    ->setTitle('AAPL last 90 days')
    ->setTheme(FastChart\Chart::THEME_DARK)
    ->setOhlcv($ohlcvRows)              // [[ts, o, h, l, c, v], …]
    ->setMovingAverages([20, 50, 200])
    ->setVolumePane(true)
    ->setCandleStyle(FastChart\Chart::STYLE_HOLLOW)
    ->draw($canvas);

imagepng($canvas, '/tmp/aapl.png');
```

## 📊 Performance

Median in-memory `renderPng()` time on a single core (Intel i9-13950HX,
PHP 8.4 NTS, default font + DPI). Same data shape per chart type at
both resolutions, alphabetical by class name.

| Chart        | 640×480 ms | 1920×1080 ms | 1080p ops/sec |
|--------------|-----------:|-------------:|--------------:|
| AreaChart    |         24 |           76 |            13 |
| BarChart     |         39 |           84 |            12 |
| BoxPlot      |         16 |           60 |            17 |
| BubbleChart  |         13 |           62 |            16 |
| ContourChart |          9 |           52 |            19 |
| Funnel       |         14 |           52 |            19 |
| GanttChart   |         18 |           61 |            16 |
| GaugeChart   |         10 |           60 |            17 |
| Heatmap      |          9 |           56 |            18 |
| LineChart    |         21 |           66 |            15 |
| LinearMeter  |          9 |           50 |            20 |
| PieChart     |         13 |           59 |            17 |
| PolarChart   |         10 |           53 |            19 |
| RadarChart   |         15 |           61 |            16 |
| ScatterChart |         17 |           60 |            17 |
| StockChart   |         21 |           68 |            15 |
| SurfaceChart |          8 |           50 |            20 |
| Treemap      |         18 |           60 |            17 |
| Waterfall    |         18 |           61 |            16 |

Every chart type renders in under 100 ms at 1920×1080 on one thread.
At dashboard-tile size (640×480), the lighter chart types break 100
renders per second per core.

Repro the numbers locally:

```sh
php -d extension=gd -d extension=./modules/fastchart.so \
    docs/bench/bench.php
```

Iteration counts via `FC_BENCH_SMALL_ITERS` (default 200) and
`FC_BENCH_LARGE_ITERS` (default 50). Bench source at
[`docs/bench/bench.php`](docs/bench/bench.php).

## What you can render

19 chart classes plus a 2-class symbology family, all under the
`FastChart\` namespace. Each name links to its rendered example image:

- **Cartesian:** [`LineChart`](docs/examples/01_line_basic.png),
  [`AreaChart`](docs/examples/27a_area_stacked.png),
  [`BarChart`](docs/examples/03_bar_grouped.png) (vertical, horizontal,
  stacked, grouped, floating, layered),
  [`ScatterChart`](docs/examples/06_scatter_trend.png),
  [`BubbleChart`](docs/examples/14_bubble.png).
- **Financial:** [`StockChart`](docs/examples/07_stock_candle_ma.png)
  with seven candle styles (`STYLE_CANDLE`, `STYLE_BAR`,
  `STYLE_DIAMOND`, `STYLE_I_CAP`, `STYLE_HOLLOW`, `STYLE_VOLUME`,
  `STYLE_VECTOR`), SMA / EMA / WMA overlays, optional volume pane and
  custom indicator panes (RSI, MACD, Bollinger, OBV, stochastic, PSAR).
- **Non-Cartesian:** [`RadarChart`](docs/examples/08_radar.png),
  [`PolarChart`](docs/examples/16_polar.png),
  [`SurfaceChart`](docs/examples/15a_surface.png),
  [`ContourChart`](docs/examples/15b_contour.png).
- **Specialised:** [`PieChart`](docs/examples/05_pie_donut.png) (with
  optional donut hole + leader lines),
  [`GaugeChart`](docs/examples/10_gauge.png),
  [`LinearMeter`](docs/examples/36a_linear_meter_horizontal.png),
  [`GanttChart`](docs/examples/17_gantt.png),
  [`BoxPlot`](docs/examples/09_boxplot.png),
  [`Treemap`](docs/examples/32_treemap.png),
  [`Funnel`](docs/examples/33_funnel.png),
  [`Waterfall`](docs/examples/34_waterfall.png),
  [`Heatmap`](docs/examples/35_heatmap.png).
- **Symbology:** [`Code128`](docs/examples/41a_code128_alphanumeric.png)
  (1D barcode, ISO/IEC 15417, auto-switching A/B/C subsets, optional
  human-readable text), [`QrCode`](docs/examples/42b_qrcode_ecc_m.png)
  (2D matrix code, ISO/IEC 18004, ECC L/M/Q/H, versions 1..40).

Cross-cutting features available on most chart types:

- TrueType / OpenType labels via `setFontPath()` (and per-role
  `setTitleFont()`, `setAxisFont()`, `setLabelFont()`).
- Light and dark themes (`THEME_LIGHT`, `THEME_DARK`); per-series colors
  via `setSeriesColors()`; full custom palettes via `setPalette()`.
- Legend positioning (`LEGEND_TOP_RIGHT`, `_TOP_LEFT`, `_BOTTOM_RIGHT`,
  `_BOTTOM_LEFT`, `_NONE`).
- Annotations: plot bands, vertical bands, horizontal / vertical lines,
  text labels, icon plots, error bars, zones.
- Strict-mode input validation (`setStrict(true)` rejects malformed
  series with a `TypeError` instead of silently coercing to NaN).
- Background images, drop shadows, anti-aliased lines and markers.
- Image map output (`getImageMap()` returns category-aligned
  rectangles for HTML overlay).

## Examples

A gallery of code + rendered chart pairs lives in
[`docs/README.md`](docs/README.md). Forty-two runnable scripts in
[`docs/examples/`](docs/examples/) regenerate the images and exercise
every public method on the API surface.

## Public classes

All under the `FastChart\` namespace:

- `Chart`: abstract base. Carries shared geometry / theme / font /
  legend / annotation setters, the `version()` static, and the chart-
  family enums (themes, candle styles, legend positions, line styles,
  marker styles, MA kinds).
- `LineChart`, `AreaChart`, `BarChart`, `ScatterChart`,
  `BubbleChart`: series-based plots.
- `PieChart`: slice-based, with optional donut hole.
- `StockChart`: OHLC(V) candlesticks, moving-average overlays,
  volume + indicator panes.
- `RadarChart`, `PolarChart`, `SurfaceChart`,
  `ContourChart`: non-Cartesian plots.
- `GaugeChart`, `LinearMeter`: single-value readouts with zoned ranges.
- `GanttChart`: time-axis task bars with dependency links and
  milestones.
- `BoxPlot`: five-number summaries with per-category outliers.
- `Treemap`, `Funnel`, `Waterfall`: value-encoded layouts (rectangle
  packing, stage drop-off, signed-delta running totals).
- `Heatmap`: 2D grid with linear color-ramp interpolation.

Every setter returns `static`, so a single fluent expression configures
and emits a chart. `draw($canvas)` returns the same `\GdImage` for the
same reason.

The Symbol family lives parallel to `Chart` (no shared base — axes /
palettes / plot rect have no meaning for a barcode):

- `Symbol`: abstract base for all 1D + 2D codes. Carries shared
  setters: `setSize()`, `setData()`, `setQuietZone()`, `setForeground()`,
  `setBackground()`, `setTransparentBackground()`, `setDpi()`, plus
  the same `renderPng()` / `renderJpeg()` / `renderWebp()` /
  `renderGif()` / `renderAvif()` / `renderToFile()` helpers as `Chart`.
  No `draw(\GdImage)` entry — symbol renders are produced fresh each
  call; reload via `imagecreatefromstring()` to composite onto an
  existing canvas.
- `Barcode`: abstract 1D linear-barcode base.
- `Code128` (extends `Barcode`): ISO/IEC 15417, alphanumeric, three
  subsets (A: control + uppercase, B: full ASCII printable, C: digit
  pairs). Auto-switches between subsets to minimise encoded length;
  mod-103 checksum appended automatically. `setShowText(true)`
  renders the human-readable payload below the bars using the
  auto-detected default font.
- `QrCode` (extends `Symbol`): ISO/IEC 18004, four error-correction
  levels (`ECC_L` ~7%, `ECC_M` ~15%, `ECC_Q` ~25%, `ECC_H` ~30%),
  versions 1..40. Encoder is the vendored nayuki/QR-Code-generator
  C library. `setMinVersion()` / `setMaxVersion()` pin the symbol
  size; the encoder picks the smallest version that fits within the
  range. Input must be valid UTF-8.

## 🔗 PHP Performance Toolkit

Companion native PHP extensions for high-throughput PHP workloads:

- **[php_excel](https://github.com/iliaal/php_excel)**: native Excel I/O. 7-10× faster than PhpSpreadsheet, full XLS/XLSX with formulas, formatting, and styling. Powered by LibXL.
- **[mdparser](https://github.com/iliaal/mdparser)**: native CommonMark + GFM parser. 15-30× faster than pure-PHP alternatives, full CommonMark 0.31 compliance.
- **[php_clickhouse](https://github.com/iliaal/php_clickhouse)**: native ClickHouse client speaking the wire protocol directly. Picks up where SeasClick left off.

## License

BSD 3-Clause for the extension itself; see [`LICENSE`](LICENSE).
The vendored QR encoder under `vendor/qrcodegen/` (nayuki/QR-Code-generator,
C variant) is MIT — see [`vendor/qrcodegen/LICENSE`](vendor/qrcodegen/LICENSE).
SPDX: `(BSD-3-Clause AND MIT)`.

---

[Follow @iliaa on X](https://x.com/iliaa) • [Blog](https://ilia.ws) • If this saved you a chart-rendering microservice, ⭐ star it!
