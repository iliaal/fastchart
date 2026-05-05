<?php

/** @generate-class-entries */

namespace FastChart;

abstract class Chart
{
    public const int THEME_LIGHT = 0;
    public const int THEME_DARK  = 1;

    public const int MARKER_NONE    = 0;
    public const int MARKER_CIRCLE  = 1;
    public const int MARKER_SQUARE  = 2;
    public const int MARKER_DIAMOND = 3;
    public const int MARKER_CROSS   = 4;
    public const int MARKER_PLUS    = 5;

    public const int LEGEND_NONE         = 0;
    public const int LEGEND_TOP_RIGHT    = 1;
    public const int LEGEND_TOP_LEFT     = 2;
    public const int LEGEND_BOTTOM_RIGHT = 3;
    public const int LEGEND_BOTTOM_LEFT  = 4;

    public const int SCALE_LINEAR = 0;
    public const int SCALE_LOG    = 1;

    public const int LABEL_NONE    = 0;
    public const int LABEL_INSIDE  = 1;
    public const int LABEL_OUTSIDE = 2;

    public const int STYLE_CANDLE  = 0;
    public const int STYLE_BAR     = 1;
    public const int STYLE_DIAMOND = 2;
    public const int STYLE_I_CAP   = 3;
    public const int STYLE_HOLLOW  = 4;
    public const int STYLE_VOLUME  = 5;
    public const int STYLE_VECTOR  = 6;

    /** Border-side bitmask for setBorderSides(). OR them together. */
    public const int BORDER_NONE   = 0;
    public const int BORDER_LEFT   = 1;
    public const int BORDER_RIGHT  = 2;
    public const int BORDER_TOP    = 4;
    public const int BORDER_BOTTOM = 8;
    public const int BORDER_ALL    = 15;

    /** Line interpolation modes for setLineInterpolation(). */
    public const int INTERP_LINEAR = 0;
    public const int INTERP_SMOOTH = 1;

    /** Tick mode for setTickMode(). */
    public const int TICK_NONE   = 0;
    public const int TICK_LABELS = 1;
    public const int TICK_POINTS = 2;
    public const int TICK_BOTH   = 3;

    /** Stacking mode for BarChart::setStackMode() / AreaChart. */
    public const int STACK_SUM    = 0;
    public const int STACK_BESIDE = 1;
    public const int STACK_LAYER  = 2;

    /** Pie slice-label positions (extends LABEL_INSIDE / OUTSIDE / NONE). */
    public const int LABEL_LEFT  = 3;
    public const int LABEL_RIGHT = 4;

    /** Line dash style for setLineStyle(). */
    public const int LINE_SOLID  = 0;
    public const int LINE_DASHED = 1;
    public const int LINE_DOTTED = 2;

    /** Gradient direction for setGradientFill(). */
    public const int GRADIENT_VERTICAL   = 0;
    public const int GRADIENT_HORIZONTAL = 1;

    /** Date-axis stride units for setDateAxisStride(). */
    public const int DATE_DAY     = 0;
    public const int DATE_WEEK    = 1;
    public const int DATE_MONTH   = 2;
    public const int DATE_QUARTER = 3;
    public const int DATE_YEAR    = 4;

    /**
     * Optionally pass canvas dimensions at construction so callers
     * can skip the `imagecreatetruecolor()` step entirely when they
     * use the renderXxx() shortcuts. Both `null` keeps the default
     * 800 x 600. setSize() still works and overrides per-instance.
     */
    public function __construct(?int $width = null, ?int $height = null) {}

    public static function version(): string {}

    public function setSize(int $width, int $height): static {}
    public function setTitle(string $title): static {}
    public function setTheme(int $theme): static {}
    public function setBackgroundColor(int $rgb): static {}
    public function setPlotBackgroundColor(int $rgb): static {}
    public function setSeriesColors(array $colors): static {}
    public function setFontPath(string $path): static {}
    public function setFontSize(float $size): static {}
    public function setCategoryLabels(array $labels): static {}
    public function setLegendPosition(int $position): static {}
    public function setYAxisScale(int $scale): static {}
    public function setStrict(bool $strict): static {}

    /**
     * X-axis title rendered below the X-axis labels. Empty string
     * suppresses. Pie ignores. Default: "" (no title).
     */
    public function setXAxisTitle(string $title): static {}

    /**
     * Y-axis title rendered rotated 90deg to the left of the
     * Y-axis labels. Empty string suppresses.
     */
    public function setYAxisTitle(string $title): static {}

    /**
     * Rotate the X-axis tick labels. 0, 45, or 90 degrees only;
     * other values raise \ValueError. Useful when long date or
     * category labels overlap horizontally.
     */
    public function setXAxisLabelAngle(int $degrees): static {}

    /**
     * Force Y-axis bounds and (optionally) tick interval. Pass
     * null for any argument to keep the auto-computed value.
     * Forced ranges still go through "nice" tick rounding unless
     * `$interval` is supplied.
     */
    public function setYAxisRange(?float $min = null, ?float $max = null, ?float $interval = null): static {}

    /**
     * Enable a secondary Y axis on the right side of the plot.
     * Series can opt into the right axis via an `'axis' => 'right'`
     * key in the series dict (default 'left'). Independent value
     * range and tick scale per axis. Currently honored on
     * `LineChart` and `AreaChart`; other types silently ignore.
     */
    public function setSecondaryYAxis(bool $enabled): static {}

    public function addHorizontalLine(float $value, ?string $label = null, ?int $color = null): static {}
    public function addVerticalLine(float $position, ?string $label = null, ?int $color = null): static {}

    /**
     * Per-element color overrides. Each takes a 24-bit RGB int or
     * -1 to revert to the theme palette default.
     */
    public function setAxisColor(int $rgb): static {}
    public function setGridColor(int $rgb): static {}
    public function setBorderColor(int $rgb): static {}
    public function setTextColor(int $rgb): static {}

    /**
     * Per-element font overrides. `setTitleFont` is used for the
     * chart title; `setAxisFont` for axis tick labels and axis
     * titles; `setLabelFont` for category labels, value labels,
     * and pie slice labels. Pass null path to keep using the
     * global setFontPath() font; pass 0.0 size to keep the
     * computed default. The two arguments are independent.
     */
    public function setTitleFont(?string $path = null, ?float $size = null): static {}
    public function setAxisFont(?string $path = null, ?float $size = null): static {}
    public function setLabelFont(?string $path = null, ?float $size = null): static {}

    /**
     * Show numeric value labels next to each data point. For line
     * and scatter, labels appear above each marker; for bar, above
     * each bar. The optional sprintf format applies to each value
     * (default "%g"). No-op for pie (use setSliceLabelFormat).
     */
    public function setShowValues(bool $show, string $format = '%g'): static {}

    /**
     * Render the canvas with a transparent background. The PNG /
     * WebP / AVIF outputs preserve the alpha channel; JPEG and GIF
     * collapse to white. Default: false.
     */
    public function setTransparentBackground(bool $enabled): static {}

    /**
     * Composite a background image onto the canvas before drawing
     * any chart elements. Path is resolved through PHP's filesystem
     * policy (`open_basedir`). Supported source formats: PNG, JPEG,
     * WebP, GIF. The image is scaled to fill the entire canvas.
     */
    public function setBackgroundImage(string $path): static {}

    /**
     * Line interpolation mode. `INTERP_LINEAR` (default) connects
     * data points with straight segments. `INTERP_SMOOTH` uses
     * Catmull-Rom spline interpolation for a curved through-the-
     * points appearance. Affects LineChart series, AreaChart top
     * edges, and StockChart SMA overlays.
     */
    public function setLineInterpolation(int $mode): static {}

    /**
     * Force the plot rectangle to specific canvas coordinates,
     * bypassing the auto-layout that reserves space for title /
     * axes / labels. Useful for pixel-perfect chart placement
     * inside a larger composition. Coordinates are inclusive.
     * Pass any negative width / height to revert to auto-layout.
     */
    public function setPlotRect(int $x0, int $y0, int $x1, int $y1): static {}

    /**
     * Which sides of the plot border to draw. Bitwise OR of
     * `BORDER_LEFT` / `BORDER_RIGHT` / `BORDER_TOP` / `BORDER_BOTTOM`,
     * or `BORDER_ALL` (default) / `BORDER_NONE`. The Y-axis line
     * is drawn separately and is not affected by this setting.
     */
    public function setBorderSides(int $sides): static {}

    /**
     * Add a series that draws on top of the primary chart's data,
     * using the same X axis and (by default) the same Y axis.
     * Lets a `BarChart` carry a trend line, an `AreaChart` carry
     * a target band, etc. -- the v0.x equivalent of GDChart's
     * `COMBO_*` chart types.
     *
     * `$type` is `'line'` or `'area'`. `$values` is a list of
     * numeric values parallel to the primary's categories (or
     * matching the candle count for `StockChart`). `$opts` keys:
     *   - `'color'`     => int 0xRRGGBB
     *   - `'label'`     => string (legend label)
     *   - `'thickness'` => int (line width, default 2)
     *   - `'axis'`      => `'left'` (default) or `'right'` for
     *                      secondary Y axis
     */
    public function addOverlaySeries(string $type, array $values, ?array $opts = null): static {}

    /**
     * Show or hide the X axis line, ticks, and labels entirely.
     * Default true. The X-axis title remains independently controlled
     * via setXAxisTitle().
     */
    public function setXAxisVisible(bool $visible): static {}

    /**
     * Show or hide the Y axis line, ticks, and labels entirely.
     * Default true. The Y-axis title remains independently controlled
     * via setYAxisTitle().
     */
    public function setYAxisVisible(bool $visible): static {}

    /**
     * sprintf format string for Y-axis tick labels (e.g., '$%.2f',
     * '%d ms'). Empty string reverts to auto-formatting based on
     * the tick step. Receives the numeric tick value as its sole
     * argument.
     */
    public function setYAxisLabelFormat(string $format): static {}

    /**
     * sprintf format string for X-axis tick labels when the X axis
     * is numeric (Stock charts, scatter). Empty string reverts to
     * auto-formatting. No effect on category-axis charts -- use
     * setCategoryLabels() instead.
     */
    public function setXAxisLabelFormat(string $format): static {}

    /**
     * Tick rendering mode: TICK_NONE suppresses both ticks and
     * labels, TICK_LABELS draws only labels (no tick marks),
     * TICK_POINTS draws only tick marks (no labels), TICK_BOTH
     * (default) draws both.
     */
    public function setTickMode(int $mode): static {}

    /**
     * Bar fill width as a percent of the slot width (1..100).
     * 100 means bars touch each other; the GDChart default was 75.
     * Affects BarChart and StockChart candle bodies.
     */
    public function setBarWidth(int $percent): static {}

    /**
     * Edge (outline) color for filled shapes -- bars, area fills,
     * pie slices. 24-bit RGB or -1 for no outline (default).
     */
    public function setEdgeColor(int $rgb): static {}

    /**
     * When the data range crosses zero, draw a horizontal "shelf"
     * line at y=0 in axis color. Helps separate negative bars
     * from positive ones visually. Default false.
     */
    public function setZeroShelf(bool $enabled): static {}

    /**
     * Render only every Nth X-axis label, starting from index 0.
     * Useful when many category labels overlap. Pass 1 (default)
     * to render all labels.
     */
    public function setXLabelStride(int $stride): static {}

    /**
     * Title for the secondary Y axis (when setSecondaryYAxis(true)).
     * Rendered rotated 90deg to the right of the right-axis labels.
     */
    public function setSecondaryYAxisTitle(string $title): static {}

    /**
     * Thumbnail mode auto-shrinks fonts and elides labels for tiny
     * preview renders. Useful for sparkline-style or grid-of-charts
     * layouts. Default false.
     */
    public function setThumbnailMode(bool $enabled): static {}

    /**
     * Per-element text color overrides. Each takes a 24-bit RGB or
     * -1 to fall through to setTextColor() / theme. setTitleColor
     * is the chart title; setAxisLabelColor is tick labels;
     * setAxisTitleColor is X/Y axis titles.
     */
    public function setTitleColor(int $rgb): static {}
    public function setAxisLabelColor(int $rgb): static {}
    public function setAxisTitleColor(int $rgb): static {}

    /**
     * Add a free-floating text annotation at canvas coordinates
     * (`$x`, `$y` are pixel positions in the rendered image).
     * Useful for callouts, watermarks, or labeled regions. Color
     * defaults to the configured text color.
     */
    public function addTextAnnotation(string $text, int $x, int $y, ?int $color = null): static {}

    /**
     * Line dash style for line series and overlay lines:
     * `LINE_SOLID` (default), `LINE_DASHED`, `LINE_DOTTED`.
     * Doesn't affect grid, axis, or annotation lines.
     */
    public function setLineStyle(int $style): static {}

    /**
     * Apply a linear gradient to filled shapes (bars, area fills,
     * pie slices). `$from` is the color at the top (or left, for
     * horizontal); `$to` is at the bottom (or right). Pass -1 for
     * `$from` to disable gradient and revert to solid fills.
     */
    public function setGradientFill(int $from, int $to = -1, int $direction = Chart::GRADIENT_VERTICAL): static {}

    /**
     * Add a drop shadow behind filled shapes and text. `$offsetX`
     * and `$offsetY` are shadow displacement in pixels. `$color`
     * defaults to a 50% opacity black. Pass `setDropShadow(0, 0)`
     * to disable.
     */
    public function setDropShadow(int $offsetX, int $offsetY, ?int $color = null): static {}

    /**
     * Calendar-aware date-axis tick stride for charts that use a
     * Unix-timestamp X axis (StockChart, etc.). `$unit` is one of
     * `DATE_DAY` / `DATE_WEEK` / `DATE_MONTH` / `DATE_QUARTER` /
     * `DATE_YEAR`; `$every` is the multiplier (e.g. `every=2,
     * unit=DATE_WEEK` = a tick every 14 days, snapped to Mondays).
     * Pass `$every = 0` to revert to auto-density labels.
     */
    public function setDateAxisStride(int $unit, int $every = 1): static {}

    abstract public function draw(\GdImage $canvas): \GdImage;

    /** Render to PNG bytes at the configured size. */
    public function renderPng(): string {}

    /** Render to JPEG bytes. `$quality` is 1..100. */
    public function renderJpeg(int $quality = 90): string {}

    /** Render to WebP bytes. `$quality` is 0..100. */
    public function renderWebp(int $quality = 90): string {}

    /** Render to GIF bytes. */
    public function renderGif(): string {}

    /**
     * Render to AVIF bytes. `$quality` is 0..100. Raises
     * \RuntimeException if libgd was built without AVIF support.
     */
    public function renderAvif(int $quality = 60): string {}

    /**
     * Render and write directly to a file. Format is inferred from
     * the path extension: `.png` / `.jpg` / `.jpeg` / `.webp` /
     * `.gif` / `.avif`. `$quality` only applies to JPEG / WebP /
     * AVIF outputs. Returns the byte count written. Honors
     * `open_basedir`.
     */
    public function renderToFile(string $path, int $quality = 90): int {}
}

final class LineChart extends Chart
{
    public function setSeries(array $series): static {}
    public function setMarkerStyle(int $style): static {}
    public function setMarkerSize(int $size): static {}

    /**
     * Per-point error bar magnitudes. `$errors` is a flat list
     * parallel to the primary series: each entry is either a single
     * positive number (symmetric ±error) or `[lo, hi]` for an
     * asymmetric bar. Pass `[]` to clear. Drawn as vertical stems
     * with horizontal caps in the configured axis color.
     */
    public function setErrorBars(array $errors): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

final class AreaChart extends Chart
{
    /**
     * Same data shape as `LineChart::setSeries()`. Each series is
     * filled below the line down to the zero baseline (or to the
     * Y-axis min, whichever is higher). Multi-series stacks by
     * default; pass `setStacked(false)` for overlapping translucent
     * fills.
     */
    public function setSeries(array $series): static {}

    public function setStacked(bool $stacked): static {}

    /**
     * Fill alpha for non-stacked overlapping areas (0..127, where
     * 127 is fully transparent and 0 is fully opaque, matching
     * libgd's alpha convention). Default 64. Stacked areas are
     * always opaque.
     */
    public function setFillOpacity(int $alpha): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

final class BarChart extends Chart
{
    public function setSeries(array $series): static {}
    public function setStacked(bool $stacked): static {}

    /**
     * Stacking algorithm for multi-series bars when stacked mode is
     * on: STACK_SUM (default) cumulates bars vertically;
     * STACK_LAYER overlays bars front-to-back at the same baseline
     * with translucent fills; STACK_BESIDE is equivalent to
     * setStacked(false) (side-by-side groups).
     */
    public function setStackMode(int $mode): static {}

    /**
     * Switch the chart to floating-bar mode: each series entry must
     * be `[$min, $max]` rather than a scalar, and bars are drawn
     * between min and max instead of from zero. Useful for Gantt-style
     * timelines, salary-range plots, etc.
     */
    public function setFloating(bool $enabled): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

final class PieChart extends Chart
{
    public function setSlices(array $slices): static {}
    public function setDonutHoleRatio(float $ratio): static {}

    /**
     * Per-slice radial offset in pixels, indexed by slice index.
     * Pass `[0 => 20]` to push the first slice 20px outward to
     * highlight it. Slices not mentioned stay at radius 0.
     */
    public function setExplode(array $offsets): static {}

    /**
     * Where slice labels render. `LABEL_INSIDE` (default) places
     * labels at the radial midpoint inside the slice.
     * `LABEL_OUTSIDE` places labels just past the slice edge with
     * a short leader line. `LABEL_NONE` suppresses labels.
     */
    public function setSliceLabelPosition(int $position): static {}

    /**
     * sprintf format string for slice labels. Receives the
     * percentage value as its sole argument. Default `"%.0f%%"`.
     */
    public function setSliceLabelFormat(string $format): static {}

    /**
     * Aggregate slices below `$percent` of the total into a single
     * "Other" slice (or the configurable label via the second
     * argument). Pass 0 to disable (default). Useful when a long
     * tail of small slices clutters the pie.
     */
    public function setOtherThreshold(float $percent, string $label = 'Other'): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

final class ScatterChart extends Chart
{
    public function setPoints(array $points): static {}
    public function setMarkerStyle(int $style): static {}
    public function setMarkerSize(int $size): static {}

    /**
     * Overlay a least-squares regression curve over the scatter
     * points. `$degree = 1` (default) is the linear fit; 2..5
     * fits a polynomial of that order. Pass false (default) to
     * suppress.
     */
    public function setTrendLine(bool $enabled, ?int $color = null, int $degree = 1): static {}

    /**
     * Per-point error bar magnitudes parallel to setPoints().
     * Each entry is either a single positive number (symmetric)
     * or `[lo, hi]` (asymmetric). Pass `[]` to clear.
     */
    public function setErrorBars(array $errors): static {}

    /**
     * After a render, return an HTML imagemap describing the
     * clickable region for each scatter point. Each entry has a
     * `'url'` and `'title'` taken from the matching `'href'` /
     * `'tooltip'` keys on the source data. Empty string when no
     * points carry URLs. The map's `name` attribute is the supplied
     * `$name`, sanitized to alphanumeric + dash + underscore.
     */
    public function getImageMap(string $name = 'fastchart'): string {}

    public function draw(\GdImage $canvas): \GdImage {}
}

final class StockChart extends Chart
{
    public function setOhlcv(array $ohlcv): static {}
    public function setMovingAverages(array $periods): static {}
    public function setVolumePane(bool $enabled): static {}

    /**
     * Per-bar volume color override. `$colors` is an array of
     * 24-bit RGB ints parallel to the OHLCV rows -- one entry per
     * candle. When set, replaces the candle-direction up/down
     * volume coloring. Pass `[]` to revert to the default coloring.
     */
    public function setVolumeColors(array $colors): static {}

    /**
     * OHLC presentation style.
     *   - `STYLE_CANDLE` (default) — filled body + high-low wick.
     *   - `STYLE_BAR` — vertical wick + left tick at open + right
     *     tick at close (classic Western HLC bar).
     *   - `STYLE_DIAMOND` — diamond at close + high-low wick.
     *   - `STYLE_I_CAP` — wick with horizontal caps at high and low.
     *   - `STYLE_HOLLOW` — outlined-only body for bullish bars,
     *     filled body for bearish bars (TradingView convention).
     *   - `STYLE_VOLUME` — body width scales with the bar's volume
     *     relative to the rolling-average volume (requires the OHLCV
     *     row to carry a volume column).
     *   - `STYLE_VECTOR` — six-color scheme based on (direction) ×
     *     (volume strength: climax / rising / neutral). Uses the
     *     same algorithm as the pinescript "Vector Candles"
     *     indicator: climax when volume >= 2× avg or
     *     volume × (high-low) >= the running max; rising when volume
     *     >= 1.5× avg; otherwise neutral. Requires a volume column.
     */
    public function setCandleStyle(int $style): static {}

    public function addIndicatorPane(string $name, array $values, ?array $opts = null): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

/**
 * Spider / radar chart. Each axis radiates from the center; one
 * polygon per series threads its values across all axes.
 */
final class RadarChart extends Chart
{
    /**
     * Either a flat `[v0, v1, v2, ...]` (single series) or a list of
     * `['data' => [...], 'label' => 'name', 'color' => 0xRRGGBB]` for
     * multi-series. All series must have the same length, which fixes
     * the number of axes (use setCategoryLabels for axis names).
     */
    public function setSeries(array $series): static {}

    /** Force a maximum value for the radial scale. 0 = auto. */
    public function setMaxValue(float $max): static {}

    /**
     * Fill the radar polygon area in the series color (translucent).
     * Default true. Pass false for line-only spider plots.
     */
    public function setFilled(bool $filled): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

/**
 * Bubble chart. Each point is `[x, y, size]`, optionally
 * `[x, y, size, color]`. Size is a positive radius in pixels.
 */
final class BubbleChart extends Chart
{
    public function setPoints(array $points): static {}
    public function draw(\GdImage $canvas): \GdImage {}
}

/**
 * Surface / heatmap. Data is a 2D array (rows of columns) of numeric
 * values. Each cell is colored by its value via a configurable color
 * ramp.
 */
final class SurfaceChart extends Chart
{
    /**
     * 2D grid of values: `[[v00, v01, ...], [v10, v11, ...]]`. Rows
     * paint top-to-bottom; columns left-to-right.
     */
    public function setGrid(array $grid): static {}

    /**
     * Two-stop color ramp (low value -> high value). 24-bit RGB ints.
     * Default cool blue -> warm red.
     */
    public function setColorRamp(int $low, int $high): static {}

    /**
     * Show the numeric value inside each cell. Default false.
     */
    public function setShowCellValues(bool $show, string $format = '%g'): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

/**
 * Gauge / dial readout: a single value within `[min, max]`, drawn as
 * a 180° arc with a needle. Optional colored zones partition the arc.
 */
final class GaugeChart extends Chart
{
    public function setValue(float $value): static {}

    /**
     * Numeric range of the gauge. Default `[0.0, 100.0]`.
     */
    public function setRange(float $min, float $max): static {}

    /**
     * Colored zones along the arc. Each zone is
     * `['from' => float, 'to' => float, 'color' => int]`. Zones not
     * covering the full range fill in with the theme accent color.
     */
    public function setZones(array $zones): static {}

    /**
     * sprintf format for the central value label. Default `"%.1f"`.
     */
    public function setValueFormat(string $format): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

/**
 * Gantt chart: a horizontal-bar timeline. Each task has a name, a
 * start and end timestamp, an optional color, and an optional list
 * of dependency-task indices that draw an arrow from the dependency
 * task's end to this task's start.
 */
final class GanttChart extends Chart
{
    /**
     * Tasks: list of dicts with keys `'name'`, `'start'` (Unix
     * timestamp), `'end'` (Unix timestamp), optional `'color'`
     * (0xRRGGBB), optional `'milestone'` (bool, default false --
     * draws a diamond at end instead of a bar), optional
     * `'depends'` (list of indices into the same tasks array).
     */
    public function setTasks(array $tasks): static {}

    /**
     * Force the time-axis range. Pass null for either to auto-fit.
     */
    public function setTimeRange(?int $start = null, ?int $end = null): static {}

    /** Show task name labels on / next to the bars. Default true. */
    public function setShowTaskLabels(bool $show): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

/**
 * Box-and-whisker plot. Each category renders a box from Q1..Q3
 * with a median line and whiskers extending to the min/max, plus
 * optional outlier dots.
 */
final class BoxPlot extends Chart
{
    /**
     * One box per entry. Each entry is either a flat
     * `[$min, $q1, $median, $q3, $max]` or a dict with the same
     * keys plus optional `'outliers' => [v1, v2, ...]` and
     * `'label' => 'name'`.
     */
    public function setBoxes(array $boxes): static {}

    /**
     * Box width as a percent of slot width (1..100). Default 60.
     */
    public function setBoxWidth(int $percent): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

/**
 * Polar plot: continuous angular coordinate (radar's parent shape).
 * Points are `[angle_deg, radius]` and connect into a polygon.
 */
final class PolarChart extends Chart
{
    /**
     * Points (single series) or list of series with
     * `['data' => [[deg, r], ...], 'label' => 'name', 'color' => int]`.
     */
    public function setSeries(array $series): static {}

    /** Force the radial scale. 0 = auto. */
    public function setMaxRadius(float $max): static {}

    /** Fill the polygon area in the series color (translucent). */
    public function setFilled(bool $filled): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

/**
 * Contour plot: isolines drawn through a 2D grid of values.
 * Each level produces a closed-or-open curve where the grid value
 * equals that level (marching-squares algorithm).
 */
final class ContourChart extends Chart
{
    /** 2D grid of numeric values. */
    public function setGrid(array $grid): static {}

    /**
     * Levels at which to draw isolines. If empty, the renderer picks
     * 5 evenly-spaced levels between the min and max grid values.
     */
    public function setLevels(array $levels): static {}

    /**
     * Color the area between isolines on a low->high color ramp.
     * Default true. Pass false for line-only contours.
     */
    public function setFilled(bool $filled): static {}

    public function setColorRamp(int $low, int $high): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}
