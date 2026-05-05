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
     * OHLC presentation style. `STYLE_CANDLE` (default) draws a
     * filled body with a high-low wick. `STYLE_BAR` draws a
     * vertical line with a left tick at the open and a right tick
     * at the close (classic Western HLC bar). `STYLE_DIAMOND`
     * draws a diamond at the close with a high-low wick.
     * `STYLE_I_CAP` draws the wick with horizontal caps at high
     * and low.
     */
    public function setCandleStyle(int $style): static {}

    public function addIndicatorPane(string $name, array $values, ?array $opts = null): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}
