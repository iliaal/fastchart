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

    /**
     * Returns the loaded fastchart extension version, e.g. "0.1.0".
     */
    public static function version(): string {}

    /**
     * Set the canvas dimensions the chart will draw within. Default
     * 800 x 600. Out-of-bounds canvas raises a \ValueError.
     */
    public function setSize(int $width, int $height): static {}

    /**
     * Set the title rendered above the plot area. Empty string
     * suppresses the title.
     */
    public function setTitle(string $title): static {}

    /**
     * Select a built-in theme (`THEME_LIGHT`, `THEME_DARK`).
     * Per-instance setBackgroundColor / setSeriesColors override
     * specific palette slots without disturbing the rest.
     */
    public function setTheme(int $theme): static {}

    /**
     * Override the canvas background color (24-bit 0xRRGGBB).
     * Pass -1 to revert to the theme default. Affects the canvas
     * around the plot area; the plot area background tracks this
     * unless setPlotBackgroundColor() is also called.
     */
    public function setBackgroundColor(int $rgb): static {}

    /**
     * Override the plot-area background color independently of the
     * canvas background. Pass -1 to revert to the theme default.
     */
    public function setPlotBackgroundColor(int $rgb): static {}

    /**
     * Override the per-series color palette. Pass a list of up to 8
     * integers (24-bit 0xRRGGBB). Series index N rotates through the
     * supplied colors; pass [] to revert to the theme palette.
     */
    public function setSeriesColors(array $colors): static {}

    /**
     * Set the TTF font used for titles, axis labels, value tags.
     */
    public function setFontPath(string $path): static {}

    /**
     * Set the base font size in points. Default: 10.0.
     */
    public function setFontSize(float $size): static {}

    /**
     * Set the X-axis category labels for chart types that use a
     * categorical X-axis (Line, Bar). Pie/Scatter/Stock ignore this
     * setter.
     */
    public function setCategoryLabels(array $labels): static {}

    /**
     * Where to draw the legend, when one applies. Pass one of the
     * `LEGEND_*` class constants. `LEGEND_NONE` suppresses the
     * legend entirely. Default: `LEGEND_TOP_RIGHT`.
     */
    public function setLegendPosition(int $position): static {}

    /**
     * Y-axis scale: `SCALE_LINEAR` (default) or `SCALE_LOG` (base
     * 10). Log scale requires strictly-positive data; passing
     * non-positive values to a log-scaled chart raises \ValueError
     * at draw() time.
     */
    public function setYAxisScale(int $scale): static {}

    /**
     * Strict input validation. With strict mode on (default off),
     * a non-numeric value inside the data passed to setSeries /
     * setSlices / setPoints / setOhlcv triggers \TypeError on
     * draw() instead of being silently skipped.
     */
    public function setStrict(bool $strict): static {}

    /**
     * Add a horizontal reference line at the given Y value. Renders
     * as a dashed line across the plot area; if `$label` is given,
     * the label is drawn at the right edge. `$color` is 24-bit
     * 0xRRGGBB; null uses the theme axis color.
     */
    public function addHorizontalLine(float $value, ?string $label = null, ?int $color = null): static {}

    /**
     * Add a vertical reference line at the given X position. For
     * Line / Bar charts the position is interpreted as a category
     * index (0..n-1); for Scatter as a numeric x-value; for Stock
     * as a Unix timestamp.
     */
    public function addVerticalLine(float $position, ?string $label = null, ?int $color = null): static {}

    /**
     * Render the chart into the caller-supplied GD canvas and
     * return the same canvas for chaining.
     */
    abstract public function draw(\GdImage $canvas): \GdImage;

    /**
     * Convenience: draw to a fresh internal canvas at the size
     * configured via setSize() and return PNG-encoded bytes.
     * Equivalent to `imagepng()` on a `draw()` result, without the
     * ext/gd round-trip.
     */
    public function renderPng(): string {}

    /**
     * As renderPng(), but JPEG-encoded. `$quality` is 1..100
     * (default 90).
     */
    public function renderJpeg(int $quality = 90): string {}

    /**
     * As renderPng(), but WebP-encoded. `$quality` is 0..100
     * (default 90).
     */
    public function renderWebp(int $quality = 90): string {}
}

final class LineChart extends Chart
{
    public function setSeries(array $series): static {}

    /**
     * Marker shape drawn at each data point. Default
     * `MARKER_CIRCLE`. `MARKER_NONE` plots only the connecting
     * lines.
     */
    public function setMarkerStyle(int $style): static {}

    /**
     * Marker size in pixels (1..32). Default 6.
     */
    public function setMarkerSize(int $size): static {}

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

    public function draw(\GdImage $canvas): \GdImage {}
}

final class ScatterChart extends Chart
{
    public function setPoints(array $points): static {}

    /**
     * Marker shape for each point. Default `MARKER_CIRCLE`.
     */
    public function setMarkerStyle(int $style): static {}

    /**
     * Marker size in pixels (1..32). Default 7.
     */
    public function setMarkerSize(int $size): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

final class StockChart extends Chart
{
    public function setOhlcv(array $ohlcv): static {}

    public function setMovingAverages(array $periods): static {}

    public function setVolumePane(bool $enabled): static {}

    /**
     * Add a stacked indicator pane below the price (and volume)
     * pane. `$values` is parallel to the OHLCV rows -- one numeric
     * value per row in the same order. Optional `$opts`:
     *   'color'     => int 0xRRGGBB (default: rotates the palette)
     *   'reference' => float (draws a horizontal reference line at
     *                  this value, e.g. 50 for an RSI midline)
     *   'min'       => float (clamp the pane's y-range minimum)
     *   'max'       => float (clamp the pane's y-range maximum)
     * Multiple panes stack vertically. Up to 3 panes total.
     */
    public function addIndicatorPane(string $name, array $values, ?array $opts = null): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}
