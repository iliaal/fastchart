# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `StockChart::setCandleStyle` — three new presentation styles:
  `STYLE_HOLLOW` (TradingView-style: outlined bullish bodies,
  filled bearish), `STYLE_VOLUME` (body width scales with the bar's
  volume relative to the rolling-average volume; requires the OHLCV
  row to carry a volume column), and `STYLE_VECTOR` (six-color
  scheme based on direction × volume strength, using the same
  climax / rising / neutral algorithm as the pinescript "Vector
  Candles" indicator: climax = vol ≥ 2× avg or vol×range ≥ rolling
  max; rising = vol ≥ 1.5× avg; otherwise neutral).
- `FastChart\GanttChart` — project-management timeline chart. Tasks
  carry `'name'`, `'start'` (Unix ts), `'end'` (Unix ts), optional
  `'color'`, `'milestone'` (diamond at end instead of bar), and
  `'depends'` (list of task indices that draw arrow dependencies).
  `setTimeRange(start, end)` overrides auto-fit;
  `setShowTaskLabels(bool)` toggles inline task names.
- `FastChart\BoxPlot` — quartile box chart for distributions. Each
  box accepts either a flat `[min, q1, median, q3, max]` list or a
  dict with the same keys plus `'outliers' => [...]` and `'label'`.
  `setBoxWidth(int $percent)` controls box fill width.
- `FastChart\PolarChart` — continuous polar plot. Points are
  `[angle_deg, radius]`; multi-series shape with `'data' / 'label'
  / 'color'` keys also supported. `setMaxRadius(float)`,
  `setFilled(bool)`.
- `FastChart\ContourChart` — isolines through a 2D grid via
  marching squares. `setLevels(array)` for explicit threshold
  levels (auto-spaced 5 if empty). `setFilled(bool)` toggles the
  filled cell-color underlay; `setColorRamp(low, high)` configures
  the ramp.
- `LineChart::setErrorBars(array)` and
  `ScatterChart::setErrorBars(array)` — per-point ± error stems
  with caps. Each entry is a scalar (symmetric) or `[lo, hi]`
  (asymmetric).
- `ScatterChart::setTrendLine(bool, ?int $color, int $degree = 1)`
  — added the `$degree` parameter; values 2..5 fit a polynomial
  regression via least-squares normal equations with Gauss-Jordan
  partial-pivoted elimination, rendered as a 200-segment curve.
- `Chart::setDateAxisStride(int $unit, int $every = 1)` —
  calendar-aware tick stride for time-axis charts. Units:
  `DATE_DAY`, `DATE_WEEK` (snaps to Mondays), `DATE_MONTH`,
  `DATE_QUARTER`, `DATE_YEAR`. `every = 0` reverts to auto-density.
  Honored by `StockChart` and `GanttChart`.
- `Chart::getImageMap(string $name = 'fastchart')` — after
  rendering, returns an HTML `<map>` element with one `<area>`
  per data point that carried an `'href'` key (and optional
  `'tooltip'`). Currently populated by `ScatterChart`. Clicking
  in the rendered image takes the user to the corresponding URL.
- `FastChart\RadarChart` — spider/radar chart class. N axes radiating
  from the center, one polygon per series. Single-flat-list and
  multi-series shapes accepted. `setMaxValue(float)` forces the
  radial scale; `setFilled(bool)` toggles between filled translucent
  polygons and outline-only spider plots.
- `FastChart\BubbleChart` — `setPoints([[x, y, size, ?color], ...])`
  draws translucent circles whose radius scales with the size
  dimension. Uses sqrt scaling so large values don't dominate.
- `FastChart\SurfaceChart` — heatmap / 2D grid colored by value.
  `setGrid([[v, v, ...], ...])`, `setColorRamp(int $low, int $high)`
  for the two-stop ramp (default cool blue → warm red),
  `setShowCellValues(bool, ?string $format)` to print the numeric
  value inside each cell with contrast-correct text color.
- `FastChart\GaugeChart` — semi-circular dial readout with a needle
  pointing at the value. `setRange(min, max)`, `setValue(float)`,
  `setZones([[from, to, color], ...])` for green/yellow/red zones,
  `setValueFormat(string)` for the central label.
- `Chart::setLineStyle(LINE_SOLID | LINE_DASHED | LINE_DOTTED)` —
  dash pattern for line series and overlay lines via libgd's
  `gdImageSetStyle` styled-color path. Dashed/dotted lines drop
  antialiasing because libgd doesn't compose `gdStyled` and
  `gdAntiAliased`.
- `Chart::setGradientFill(int $from, int $to = -1, int $direction = GRADIENT_VERTICAL)`
  applies a linear gradient to bar / area-fill / pie-slice shapes.
  `-1` for `$from` reverts to solid fills.
- `Chart::setDropShadow(int $offsetX, int $offsetY, ?int $color = null)`
  paints a translucent shadow at the offset position behind each
  filled shape. `setDropShadow(0, 0)` disables.
- `ScatterChart::setTrendLine(bool, ?int $color)` overlays a
  least-squares linear regression line through the scatter points.
- `Chart::__construct(?int $width = null, ?int $height = null)` —
  optional canvas size at construction so callers using
  `renderPng()` / `renderToFile()` don't need a separate
  `imagecreatetruecolor()` step. `setSize()` still works and
  overrides per-instance.
- `FastChart\AreaChart` — new chart class. Filled-line areas with
  the same data shape as `LineChart`. `setStacked(true)` (default)
  paints opaque cumulative areas; `setStacked(false)` paints
  translucent overlapping fills with `setFillOpacity(0..127)`.
  Honors null-sentinel gaps and category labels.
- `Chart::setXAxisTitle(string)` and `Chart::setYAxisTitle(string)`
  render axis titles below the X labels and rotated 90° on the
  left side of the Y labels. Empty string suppresses.
- `Chart::setXAxisLabelAngle(int $degrees)` rotates X-axis tick
  labels by 0, 45, or 90 degrees. Useful when long date or
  category labels overlap horizontally.
- `Chart::setYAxisRange(?float $min, ?float $max, ?float $interval)`
  forces Y-axis bounds and (optionally) tick interval. Pass null
  for any argument to keep the auto-computed value.
- `Chart::setSecondaryYAxis(bool)` enables a right-side Y axis
  with an independent value range. Series opt in via
  `'axis' => 'right'` in the series dict. Currently honored on
  `LineChart` and `AreaChart`.
- Null values in `LineChart::setSeries()` and
  `AreaChart::setSeries()` data render as polyline gaps (intentional
  missing data). Strict mode tolerates `null` while still
  rejecting other non-numeric types.
- `PieChart::setExplode([slice_index => offset_pixels])` displaces
  individual slices radially outward.
- `PieChart::setSliceLabelPosition(LABEL_INSIDE | LABEL_OUTSIDE | LABEL_NONE)`
  and `PieChart::setSliceLabelFormat(string $sprintf_format)` for
  labels with leader lines or fully suppressed labels.
- `StockChart::setCandleStyle(STYLE_CANDLE | STYLE_BAR | STYLE_DIAMOND | STYLE_I_CAP)`
  — four OHLC presentation styles. `STYLE_CANDLE` (default) is the
  filled body + wick. `STYLE_BAR` is the classic Western HLC bar
  (vertical line + open tick on the left + close tick on the
  right). `STYLE_DIAMOND` is a diamond at the close + wick.
  `STYLE_I_CAP` is the high-low wick with horizontal caps.
- Per-point ext colors. `LineChart` and `BarChart` series accept
  an optional `'colors' => [int, int, ...]` array parallel to the
  data; each value is a 24-bit RGB and overrides the series
  palette color for that data point. Useful for highlighting
  specific bars or markers.
- `Chart::renderGif()` and `Chart::renderAvif(int $quality = 60)` —
  GIF and AVIF output formats. AVIF requires libgd built with
  AVIF support; raises a runtime exception on stripped builds.
- `Chart::renderToFile(string $path, int $quality = 90)` — render
  and write directly to a file. Format is inferred from the path
  extension (`.png` / `.jpg` / `.jpeg` / `.webp` / `.gif` /
  `.avif`). Honors `open_basedir`. Returns the byte count
  written.
- Per-element color overrides: `setAxisColor(int)`,
  `setGridColor(int)`, `setBorderColor(int)`, `setTextColor(int)`.
  24-bit `0xRRGGBB` or `-1` to revert to the theme palette.
- Per-element font overrides: `setTitleFont(?string $path, ?float $size)`,
  `setAxisFont(?string $path, ?float $size)`,
  `setLabelFont(?string $path, ?float $size)`. Each role inherits
  from the global `setFontPath()` / `setFontSize()` when null.
- `setShowValues(bool $on, string $format = '%.2f')` paints
  numeric value labels above markers (Line, Scatter) and bars
  (Bar, Area).
- `PieChart::setOtherThreshold(float $pct, string $label = 'Other')`
  collapses slices below the percent threshold into a single
  aggregated slice. `0.0` disables.
- `setTransparentBackground(bool)` keeps the canvas alpha so
  PNG / WebP / AVIF outputs preserve transparency.
- `setBackgroundImage(string $path)` composites a source image as
  the canvas background; the plot area stays opaque on top.
  Empty path clears; missing file silently no-ops.
- `setLineInterpolation(INTERP_LINEAR | INTERP_SMOOTH)` switches
  Line and Area top-edge segments to Catmull-Rom curves.
- `StockChart::setVolumeColors(array)` per-bar override of the
  default red/green volume coloring. Empty array reverts.
- `setPlotRect(int x0, int y0, int x1, int y1)` bypasses
  auto-layout and forces a hard plot rectangle. Negative width
  or height reverts to auto-layout.
- `setBorderSides(int)` selects which sides of the plot frame to
  draw via `BORDER_NONE | BORDER_LEFT | BORDER_RIGHT | BORDER_TOP |
  BORDER_BOTTOM | BORDER_ALL` bitmask. Default `BORDER_ALL`.
- `addOverlaySeries(string $type, array $data, array $options = [])`
  layers a `'line'` or `'area'` overlay onto Line / Bar / Area /
  Stock charts (the GDChart `COMBO_*` equivalent). Options:
  `color`, `thickness`, `label`.
- `setXAxisVisible(bool)` / `setYAxisVisible(bool)` toggle axis
  line, ticks, and labels independently. Axis titles remain
  controlled by `setXAxisTitle` / `setYAxisTitle`.
- `setYAxisLabelFormat(string $sprintf)` and
  `setXAxisLabelFormat(string)` override the auto-formatted tick
  labels (e.g., `'$%.2f'`). Empty string reverts.
- `setTickMode(int)` selects tick rendering: `TICK_NONE`,
  `TICK_LABELS`, `TICK_POINTS`, `TICK_BOTH` (default).
- `setBarWidth(int $percent)` — bar fill width as a percent of
  slot width (1..100, default 100). Affects `BarChart` and
  `StockChart` candle bodies.
- `setEdgeColor(int $rgb)` draws an outline around filled shapes
  (bars, area fills, pie slices). `-1` disables (default).
- `setZeroShelf(bool)` paints a horizontal axis-color line at
  `y=0` when the data range crosses zero.
- `setXLabelStride(int $n)` renders only every Nth X-axis label
  (1..1000). Multiplies on top of auto-density elision.
- `setSecondaryYAxisTitle(string)` mirrors `setYAxisTitle` on the
  right-hand secondary axis (rendered when `setSecondaryYAxis(true)`).
- `setThumbnailMode(bool)` shrinks fonts and elides labels for
  sparkline-size renders.
- `setTitleColor(int)`, `setAxisLabelColor(int)`,
  `setAxisTitleColor(int)` — per-element text color overrides
  on top of `setTextColor` / theme. `-1` falls through.
- `setXAxisFont`, `setYAxisFont`, `setXAxisTitleFont`,
  `setYAxisTitleFont`, `setAnnotationFont` — per-axis-element
  font (path + size) overrides. Inherit from `setAxisFont` /
  `setFontPath` when null.
- `addTextAnnotation(string $text, int $x, int $y, ?int $color = null)`
  paints a free-floating label at canvas pixel coordinates.
- `BarChart::setStackMode(int)` — `STACK_SUM` (default,
  cumulative), `STACK_BESIDE` (alias for `setStacked(false)`),
  `STACK_LAYER` (translucent bars at the same baseline).
- `BarChart::setFloating(bool)` — switch the chart to
  `[min, max]` data; bars draw between min and max instead of
  from zero. Useful for Gantt-style ranges.
- `PieChart::setSliceLabelPosition` extended with `LABEL_LEFT`
  and `LABEL_RIGHT` — force all slice labels to one side with
  leader lines, regardless of slice angle.

- Initial scaffold. Builds against PHP 8.3+, depends on `ext/gd`,
  links libgd directly for drawing primitives.
- Public OO surface registered: `FastChart\Chart` (abstract base),
  `FastChart\LineChart`, `BarChart`, `PieChart`, `ScatterChart`,
  `StockChart`. Fluent setters (`setSize`, `setTitle`, `setTheme`,
  `setFontPath`, `setFontSize`, `setCategoryLabels`, per-type
  `setSeries` / `setSlices` / `setPoints` / `setOhlcv`,
  `setMovingAverages`, `setVolumePane`, `setStacked`,
  `setDonutHoleRatio`).
- `Chart::version()` static returning `PHP_FASTCHART_VERSION`.
- `Chart::setCategoryLabels(array)` lets line and bar charts
  render user-supplied X-axis labels (`'Q1', 'Q2', …`) instead
  of integer indices. Pie/scatter/stock silently ignore the
  setter.
- `PieChart::setSlices()` honors an optional `'color' => 0xRRGGBB`
  key on each slice dict, overriding the palette per-slice.
  Out-of-range integers fall back to the palette without
  raising. Lets callers map slices to brand colors.
- Antialiased line segments via `gdImageSetAntiAliased` +
  `gdAntiAliased` for `LineChart` series and `StockChart` SMA
  overlays. Diagonals smooth out instead of staircasing.
- Multi-series legend in the top-right of the plot area: one row
  per labeled series with a color swatch and the series label.
  Renders for `LineChart` / `BarChart` / `ScatterChart` when at
  least two series carry a `'label'` key, and for `StockChart`
  whenever moving averages are configured (labeled `SMA(N)`).
  Single-series flat-list inputs do not paint a one-row legend.
- `Chart::setLegendPosition(int)` selects which corner of the
  plot area the legend renders in:
  `LEGEND_TOP_RIGHT` (default), `LEGEND_TOP_LEFT`,
  `LEGEND_BOTTOM_RIGHT`, `LEGEND_BOTTOM_LEFT`, `LEGEND_NONE`.
- `LineChart::setMarkerStyle(int)` and `setMarkerSize(int)`
  (same on `ScatterChart`) — `MARKER_NONE`, `MARKER_CIRCLE`
  (default), `MARKER_SQUARE`, `MARKER_DIAMOND`, `MARKER_CROSS`,
  `MARKER_PLUS`. Size in [1, 32] pixels.
- `Chart::setBackgroundColor(int)` and
  `Chart::setPlotBackgroundColor(int)` override the canvas /
  plot-area background per instance (24-bit `0xRRGGBB`, or `-1`
  to revert). Setting only the canvas background propagates to
  the plot background unless the plot setter is also called.
- `Chart::setSeriesColors(array)` overrides up to 8 series
  palette entries per instance.
- `Chart::renderPng()`, `renderJpeg(int $quality = 90)`,
  `renderWebp(int $quality = 90)` — render directly to a
  binary string at the configured `setSize()` dimensions
  without round-tripping through ext/gd's `imagepng()` /
  `imagejpeg()` / `imagewebp()`. Returns 89 50 4E 47 / FF D8 /
  RIFF…WEBP bytes ready to write to disk or stream to a client.
- `Chart::setYAxisScale(int)` toggles between linear (default)
  and base-10 logarithmic Y axis. Log scale requires
  strictly-positive data; passing zero or negative values
  raises `\ValueError` at `draw()` time. Bar charts under log
  scale require all-positive data because their zero baseline
  cannot be expressed.
- `Chart::setStrict(bool)` switches non-numeric data from a
  silent skip to a `\TypeError` on `draw()`. Off by default to
  preserve existing tolerant behavior.
- `Chart::addHorizontalLine(float $value, ?string $label, ?int $color)`
  and `Chart::addVerticalLine(float $position, ?string $label, ?int $color)`
  add dashed reference lines with optional labels. Vertical
  position is interpreted by chart type: category index for
  Line/Bar, numeric x for Scatter, Unix timestamp for Stock.
  `null` color uses the theme axis color; otherwise 24-bit RGB.
  Pie ignores annotations.
- `StockChart::addIndicatorPane(string $name, array $values, ?array $opts = null)`
  stacks an indicator sub-pane below price (and below volume if
  `setVolumePane(true)` was also called). Up to 3 panes total.
  `$opts` keys: `'color' => 0xRRGGBB`, `'reference' => float`
  (horizontal reference line, e.g. RSI midline at 50),
  `'min' => float`, `'max' => float` (clamp the pane's y-range).
  Each pane has its own y-range computed from the values.
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
