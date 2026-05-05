# fastchart

Native C PHP extension for fast chart generation. Renders line, area,
bar, scatter, bubble, pie, candlestick (stock), radar, polar, surface,
contour, gauge, gantt, and box-plot charts. Built-in `renderToFile()`
/ `renderPng()` / `renderJpeg()` / `renderWebp()` / `renderAvif()`
helpers cover the common case; `draw($canvas)` hands rendering onto a
caller-owned `\GdImage` for compositing several charts onto one image
or stamping arbitrary
[ext/gd](https://www.php.net/manual/en/book.image.php) draw calls
on top.

![Stock chart with moving averages](docs/examples/07_stock_candle_ma.png)

## Status

Working. 13 chart types, 105 public methods, 86 phpt tests. The OO
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
- `ext/gd` enabled — fastchart pulls the underlying `gdImagePtr` out
  of caller-supplied `\GdImage` zvals via the one PHPAPI symbol ext/gd
  exports.
- libgd development headers at build time (`libgd-dev` on Debian /
  Ubuntu, `gd-devel` on RHEL / Fedora).
- FreeType (already a libgd dependency on every Linux distribution) —
  used for TrueType / OpenType label rendering.

## Quick start

The shortest path is the `renderToFile()` helper — fastchart picks
the encoder from the extension:

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
    ->setTitle('AAPL — last 90 days')
    ->setTheme(FastChart\Chart::THEME_DARK)
    ->setOhlcv($ohlcvRows)              // [[ts, o, h, l, c, v], …]
    ->setMovingAverages([20, 50, 200])
    ->setVolumePane(true)
    ->setCandleStyle(FastChart\Chart::STYLE_HOLLOW)
    ->draw($canvas);

imagepng($canvas, '/tmp/aapl.png');
```

## What you can render

- **Cartesian:** `LineChart`, `AreaChart`, `BarChart` (vertical, horizontal,
  stacked, grouped, floating, layered), `ScatterChart`, `BubbleChart`.
- **Stock:** `StockChart` with seven candle styles (`STYLE_CANDLE`,
  `STYLE_BAR`, `STYLE_DIAMOND`, `STYLE_I_CAP`, `STYLE_HOLLOW`,
  `STYLE_VOLUME`, `STYLE_VECTOR`), SMA / EMA / WMA overlays, optional
  volume pane and custom indicator panes.
- **Non-Cartesian:** `RadarChart`, `PolarChart`, `SurfaceChart`,
  `ContourChart`.
- **Specialised:** `PieChart` (with optional donut hole + leader lines),
  `GaugeChart`, `GanttChart`, `BoxPlot`.

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
[`docs/README.md`](docs/README.md). Thirty-one runnable scripts in
[`docs/examples/`](docs/examples/) regenerate the images and exercise
every public method on the API surface.

## Public classes

All under the `FastChart\` namespace:

- `Chart` — abstract base. Carries shared geometry / theme / font /
  legend / annotation setters, the `version()` static, and the chart-
  family enums (themes, candle styles, legend positions, line styles,
  marker styles, MA kinds).
- `LineChart`, `AreaChart`, `BarChart`, `ScatterChart`, `BubbleChart`
  — series-based plots.
- `PieChart` — slice-based, with optional donut hole.
- `StockChart` — OHLC(V) candlesticks, moving-average overlays,
  volume + indicator panes.
- `RadarChart`, `PolarChart`, `SurfaceChart`, `ContourChart` —
  non-Cartesian plots.
- `GaugeChart`, `GanttChart`, `BoxPlot` — specialised shapes.

Every setter returns `static`, so a single fluent expression configures
and emits a chart. `draw($canvas)` returns the same `\GdImage` for the
same reason.

## License

PHP License v3.01. See [`LICENSE`](LICENSE).
