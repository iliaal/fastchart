# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Initial scaffold. Builds against PHP 8.3+, depends on `ext/gd`,
  links libgd directly for drawing primitives.
- Public OO surface registered: `FastChart\Chart` (abstract base),
  `FastChart\LineChart`, `BarChart`, `PieChart`, `ScatterChart`,
  `StockChart`. Fluent setters (`setSize`, `setTitle`, `setTheme`,
  `setFontPath`, `setFontSize`, per-type `setSeries` / `setSlices` /
  `setPoints` / `setOhlcv`, `setMovingAverages`, `setVolumePane`,
  `setStacked`, `setDonutHoleRatio`).
- `Chart::version()` static returning `PHP_FASTCHART_VERSION`.
- `LineChart::draw()` renders one or more series as connected
  segments with point markers. Accepts a flat numeric list for
  single-series, or a list of `['label' => ..., 'data' => [...]]`
  entries for multi-series; up to 8 series share the palette.
- `BarChart::draw()` renders single-series or grouped multi-series
  bars; `setStacked(true)` switches to a stacked layout that handles
  positive and negative values via separate accumulators.
- `PieChart::draw()` renders proportional slices via
  `gdImageFilledArc`. Accepts an associative `[label => value]` map
  or a list of `['label' => ..., 'value' => ...]` entries; supports
  donut overdraw via `setDonutHoleRatio()` and emits percentage
  labels for slices >= 8°.
- `ScatterChart::draw()` plots filled-disk markers at `[x, y]` data
  positions with a continuous numeric x-axis (nice-rounded ticks).
  Accepts a flat list of pairs or labeled multi-series.
- `StockChart::draw()` renders OHLC candlesticks (filled bodies,
  green up / red down, high-low wicks) on a Unix-time x-axis.
  Supports `setMovingAverages([periods, ...])` SMA overlays and an
  optional `setVolumePane(true)` sub-pane below the price pane that
  draws colored volume bars when row[5] (volume) is supplied.
- TTF text rendering via `gdImageStringFT`. The `setFontPath()`
  setter overrides the per-instance font; at MINIT, fastchart
  probes a short list of common font paths (DejaVuSans on
  Linux distros, Helvetica on macOS) and uses the first hit as the
  default, so most installs work without explicit configuration.
  `setFontPath()` enforces `php_check_open_basedir` and rejects
  embedded NULs.

### For contributors

- Source split into `fastchart.c` (module entry, base methods,
  setters), `fastchart_palette.{c,h}`, `fastchart_text.{c,h}`,
  `fastchart_axis.{c,h}`, and one `fastchart_<type>.c` per concrete
  chart class. The arginfo header is included only in
  `fastchart.c` -- per-chart files don't need it because
  `ZEND_METHOD` is itself a function definition.
- pixel-sampling phpt suite under `tests/`: `001_line_chart.phpt`
  through `005_stock_chart.phpt` plus the original
  `000_smoke.phpt`. Tests sample for palette colors inside the
  plot area to verify drawing reached the canvas, rather than
  comparing PNG bytes against a fragile baseline.

[Unreleased]: https://github.com/iliaal/fastchart/commits/master
