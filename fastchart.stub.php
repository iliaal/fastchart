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
