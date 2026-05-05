<?php

/** @generate-class-entries */

namespace FastChart;

abstract class Chart
{
    public const int THEME_LIGHT = 0;
    public const int THEME_DARK  = 1;

    /**
     * Returns the loaded fastchart extension version, e.g. "0.1.0".
     * Same value as PHP_FASTCHART_VERSION on the C side.
     */
    public static function version(): string {}

    /**
     * Set the canvas dimensions the chart will draw within. The user-
     * supplied \GdImage passed to draw() must be at least this size; an
     * out-of-bounds canvas raises a \ValueError. Defaults: 800 x 600.
     */
    public function setSize(int $width, int $height): static {}

    /**
     * Set the title text rendered above the plot area. Empty string
     * suppresses the title. Default: "" (no title).
     */
    public function setTitle(string $title): static {}

    /**
     * Select a built-in theme (color palette, grid style, font color).
     * Pass one of the THEME_* class constants. Default: THEME_LIGHT.
     */
    public function setTheme(int $theme): static {}

    /**
     * Set the TTF font file used for titles, axis labels, and value
     * tags. Required for any chart that renders text -- pass an
     * absolute path to a `.ttf` (or `.otf`) file that the calling
     * process can read. fastchart probes a small list of well-known
     * paths at MINIT (DejaVuSans on Linux, Helvetica on macOS) and
     * uses the first hit as the default; setFontPath() overrides
     * that default per-instance.
     */
    public function setFontPath(string $path): static {}

    /**
     * Set the base font size in points. Title and axis labels scale
     * relative to this. Default: 10.0.
     */
    public function setFontSize(float $size): static {}

    /**
     * Set the X-axis category labels for chart types that use a
     * categorical X-axis (Line, Bar). The array is taken in order,
     * one label per category index. Pass an empty array (the
     * default) to fall back to integer indices ("0", "1", ...).
     * Charts without a categorical X-axis (Pie, Scatter, Stock)
     * silently ignore this setter.
     */
    public function setCategoryLabels(array $labels): static {}

    /**
     * Render the chart into the caller-supplied GD canvas and return
     * the same canvas for chaining. The canvas is mutated in place;
     * existing pixels under the plot area are overwritten. Caller
     * controls dimensions, color depth, transparency, and output
     * format (imagepng / imagejpeg / imagewebp / etc.).
     */
    abstract public function draw(\GdImage $canvas): \GdImage;
}

final class LineChart extends Chart
{
    /**
     * Set one or more line series. Each series is a list of numeric
     * Y-values (X derived from index) or a list of [x, y] pairs.
     * Example: [['label' => 'A', 'data' => [1, 4, 2, 8]]].
     */
    public function setSeries(array $series): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

final class BarChart extends Chart
{
    /**
     * Set one or more bar series. Same shape as LineChart::setSeries().
     */
    public function setSeries(array $series): static {}

    /**
     * If true, multiple series stack vertically instead of grouping
     * side-by-side. Default: false.
     */
    public function setStacked(bool $stacked): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

final class PieChart extends Chart
{
    /**
     * Set the slices as ['label' => value, ...] or a list of
     * ['label' => string, 'value' => float, 'color' => ?int] entries.
     */
    public function setSlices(array $slices): static {}

    /**
     * Donut hole radius as a fraction of the outer radius. 0.0 draws
     * a solid pie; 0.5 draws a donut. Default: 0.0.
     */
    public function setDonutHoleRatio(float $ratio): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

final class ScatterChart extends Chart
{
    /**
     * Set the points as a list of [x, y] pairs, or a list of series
     * entries (['label' => 'A', 'data' => [[x, y], ...]]).
     */
    public function setPoints(array $points): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}

final class StockChart extends Chart
{
    /**
     * Set the OHLC(V) bars: a list of [timestamp, open, high, low,
     * close] or [timestamp, open, high, low, close, volume] rows.
     * Timestamps are Unix epoch seconds.
     */
    public function setOhlcv(array $ohlcv): static {}

    /**
     * Overlay simple moving averages of the given periods. Pass [] to
     * disable. Default: [].
     */
    public function setMovingAverages(array $periods): static {}

    /**
     * If true and rows include volume, render a volume sub-pane below
     * the price pane. Default: false.
     */
    public function setVolumePane(bool $enabled): static {}

    public function draw(\GdImage $canvas): \GdImage {}
}
