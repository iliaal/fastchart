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
    public const int INTERP_LINEAR      = 0;
    public const int INTERP_SMOOTH      = 1;
    public const int INTERP_STEP_AFTER  = 2;
    public const int INTERP_STEP_BEFORE = 3;

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
     * SVG text emission mode for setSvgTextMode().
     *
     * `SVG_TEXT_PATHS` (default) flattens every `<text>` element to a
     * `<g><path d="..."/></g>` group via FreeType outline
     * decomposition. The resulting SVG is self-contained — it renders
     * correctly in any SVG rasterizer, including ones that don't
     * support text (such as plutovg, which fastchart uses internally
     * for PNG/JPG/WebP output). File size grows ~30%+ vs native text.
     *
     * `SVG_TEXT_NATIVE` emits raw `<text>` elements. Smaller files;
     * requires the consumer's renderer to support SVG text and have
     * the named font (or a sans-serif fallback) available.
     *
     * `renderPng()`, `renderJpeg()`, `renderWebp()`, and
     * `renderToFile()` for raster formats always use PATHS internally
     * regardless of this setting — they go through plutovg.
     */
    public const int SVG_TEXT_NATIVE = 0;
    public const int SVG_TEXT_PATHS  = 1;

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
    /**
     * Scale of the *value* axis. SCALE_LINEAR (default) or SCALE_LOG
     * (base-10, requires strictly positive data).
     *
     * Misnomer note: the setter is named for the Y axis because every
     * chart family except horizontal BarChart puts values on Y.
     * BarChart with `setOrientation(BAR_HORIZONTAL)` puts values on
     * the X axis, but still consults this setter (i.e. on horizontal
     * bar charts, `setYAxisScale(SCALE_LOG)` actually configures the
     * X axis). The name is preserved across types so callers don't
     * need a chart-specific setter.
     */
    public function setYAxisScale(int $scale): static {}
    /**
     * Strict-mode validation for setSeries() input. When enabled,
     * non-numeric / non-null cells in the data array trigger a
     * TypeError instead of silently coercing to NaN. Default: off.
     *
     * Coverage: enforced on `LineChart::setSeries`,
     * `AreaChart::setSeries`, `BarChart::setSeries`. Other setters
     * (Pie / Scatter / Stock / BoxPlot / Radar / Polar / Bubble /
     * Gantt) silently skip malformed entries regardless of this
     * flag; those shapes are best-effort.
     */
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
     * Overlay an external image at data coordinates on the chart.
     * Currently honored on `LineChart` (x = fractional category
     * index, e.g. 2.5 = halfway between the 3rd and 4th category)
     * and `ScatterChart` (x, y both in data coordinates). Other
     * chart types silently ignore the call.
     *
     * `$path` is opened at draw time through PHP's stream layer
     * (which enforces `open_basedir` natively); missing or invalid
     * images are silently skipped so a typo doesn't abort the whole
     * render. Supported formats: PNG and JPEG only — plutosvg's
     * data-URI loader handles those two; WebP / GIF / AVIF sources
     * are skipped. `$maxWidth` / `$maxHeight` cap the display size
     * while preserving the source aspect ratio (-1 = use the source
     * dimension as-is). Source files larger than 8 MiB OR with
     * declared dimensions over 4096px on either axis OR a pixel
     * product over 16M are silently skipped to bound worker memory
     * if the path is fed from untrusted input. Up to 32 icons per
     * chart.
     */
    public function addIconAt(float $x, float $y, string $path,
                              int $maxWidth = -1,
                              int $maxHeight = -1): static {}

    /**
     * Add a horizontal plot band: a shaded Y-range region drawn behind
     * the chart data on Cartesian charts (Line / Area / Bar / Scatter
     * / Bubble / Stock / BoxPlot). Useful for "normal range" callouts
     * (e.g. healthy heart-rate band, target SLA window). `$low` and
     * `$high` are in data Y units; the renderer reorders if needed.
     * `$color` is a 24-bit RGB. `$alpha` uses the 0..127 convention
     * (0 = opaque, 127 = fully transparent), defaulting to 64 for a
     * visible-but-translucent overlay. Up to 16 bands per chart
     * (shared budget with addVerticalBand).
     */
    public function addHorizontalBand(float $low, float $high, int $color,
                                      int $alpha = 64,
                                      ?string $label = null): static {}

    /**
     * Add a vertical plot band: a shaded X-range region drawn behind
     * the chart data. Companion to addHorizontalBand. The X-axis
     * interpretation depends on the chart type:
     *   - Line / Area / Bar (vertical) / BoxPlot: fractional category
     *     index (0 = first category, n_categories = past last). Pass
     *     2.0 / 4.0 to span the 3rd through 4th category slots.
     *   - Scatter / Bubble: data X value.
     *   - Stock: unix timestamp.
     * Color / alpha / label / band cap are identical to
     * addHorizontalBand; the two share the 16-band-per-chart budget.
     */
    public function addVerticalBand(float $low, float $high, int $color,
                                    int $alpha = 64,
                                    ?string $label = null): static {}

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
     * Render the canvas with a transparent background. The PNG and
     * WebP outputs preserve the alpha channel; JPEG collapses to
     * white. Default: false.
     */
    public function setTransparentBackground(bool $enabled): static {}

    /**
     * Composite a background image onto the canvas before drawing
     * any chart elements. Path is resolved through PHP's filesystem
     * policy (`open_basedir`). Supported source formats: PNG and
     * JPEG only — plutosvg's data-URI loader handles those two and
     * the SVG embed silently skips other formats. The image is
     * scaled to fill the entire canvas.
     *
     * Source-file caps: the loader silently skips files larger
     * than 8 MiB OR with declared dimensions over 4096px on either
     * axis OR a pixel product over 16M. open_basedir is the
     * primary access gate; these caps are defense-in-depth so an
     * untrusted path can't make the decoder allocate hundreds of
     * MiB on a small file with declared 100000x100000 dimensions.
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
     * Two-stop color ramp (low value -> high value). 24-bit RGB ints.
     * Used by chart families that map a numeric range to a continuous
     * color (SurfaceChart, ContourChart). Default cool blue -> warm
     * red. No-op on chart types that don't paint a color ramp.
     */
    public function setColorRamp(int $low, int $high): static {}

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
     * Drop-shadow opacity in the 0..127 alpha space: 0 = fully
     * opaque, 127 = fully transparent. Default is 64 (~50% opacity).
     * Same convention `imagecolorallocatealpha()` uses, kept for
     * source-compat across v0.x callers.
     */
    public function setShadowAlpha(int $alpha): static {}

    /**
     * Calendar-aware date-axis tick stride for charts that use a
     * Unix-timestamp X axis (StockChart, etc.). `$unit` is one of
     * `DATE_DAY` / `DATE_WEEK` / `DATE_MONTH` / `DATE_QUARTER` /
     * `DATE_YEAR`; `$every` is the multiplier (e.g. `every=2,
     * unit=DATE_WEEK` = a tick every 14 days, snapped to Mondays).
     * Pass `$every = 0` to revert to auto-density labels.
     */
    public function setDateAxisStride(int $unit, int $every = 1): static {}

    /**
     * Output / FreeType DPI for the rendered canvas.
     *
     * fastchart owns the canvas and scales its physical pixel
     * dimensions by `dpi/96` on the raster render paths
     * (`renderPng()` / `renderJpeg()` / `renderWebp()` /
     * `renderToFile()` for those formats). The `setSize()` value is
     * the *logical* size; a chart at `setSize(640, 320)->setDpi(200)`
     * is allocated as a 1333×667 pixel canvas. Apparent layout is
     * preserved; pixel density doubles. Layout margins, tick marks,
     * and label paddings scale proportionally so labels don't crowd
     * the canvas edge. SVG output is DPI-invariant (vectors scale
     * infinitely) and reports the configured DPI in the PNG `pHYs`
     * and JPEG density metadata only.
     *
     * Common values: 96 (default, web-screen), 192 (2× retina),
     * 300 (print). Range is `[24, 1200]`.
     */
    public function setDpi(int $dpi): static {}

    /**
     * Select the SVG text emission mode used by `renderSvg()`,
     * `drawSvgFragment()`, and `renderToFile('*.svg')`. One of
     * `self::SVG_TEXT_PATHS` (default — self-contained) or
     * `self::SVG_TEXT_NATIVE` (compact, requires consumer text
     * support). Raster outputs are unaffected (they always use
     * PATHS internally).
     */
    public function setSvgTextMode(int $mode): static {}

    /**
     * Set the JPEG encode quality used by `renderJpeg()` and
     * `renderToFile('*.jpg' | '*.jpeg')`. Range 1..100; default 88.
     * Quality maps onto libjpeg-turbo's `jpeg_set_quality()` with
     * `optimize_coding=TRUE` and 4:2:0 chroma subsampling.
     */
    public function setJpegQuality(int $quality): static {}

    /** Render to PNG bytes at the configured size. */
    public function renderPng(): string {}

    /**
     * Render to JPEG bytes. `$quality` is 1..100; default 0 means
     * "use the value set via setJpegQuality()" (default 88).
     */
    public function renderJpeg(int $quality = 0): string {}

    /** Render to WebP bytes. `$quality` is 1..100. */
    public function renderWebp(int $quality = 90): string {}

    /**
     * Render and write directly to a file. Format is inferred from
     * the path extension: `.png` / `.jpg` / `.jpeg` / `.webp` /
     * `.svg`. `$quality` only applies to JPEG / WebP outputs; SVG
     * and PNG ignore it. Returns the byte count written. Honors
     * `open_basedir`. `.gif` / `.avif` extensions raise a clear
     * "dropped in v1.0" Error.
     */
    public function renderToFile(string $path, int $quality = 90): int {}

    /**
     * Render to an SVG document. Returns the full markup including
     * `<?xml ...>` prolog and `<svg>` root.
     *
     * Supported on every concrete `Chart` subclass (LineChart,
     * AreaChart, BarChart, PieChart, ScatterChart, BubbleChart,
     * RadarChart, PolarChart, SurfaceChart, ContourChart, GaugeChart,
     * GanttChart, BoxPlot, Treemap, Funnel, Waterfall, Heatmap,
     * LinearMeter, StockChart). The `Symbol` family (Code128, QrCode)
     * exposes the same method on its own abstract base.
     *
     * The output viewport matches the logical `setSize()` dimensions.
     * SVG is DPI-invariant: `setDpi()` still scales the raster canvas
     * for `renderPng()` / `renderJpeg()` / etc., but does not multiply
     * the SVG viewport — vector strokes scale infinitely, so layout
     * and text measurement stay at the 96-DPI baseline regardless of
     * the configured DPI.
     *
     * Text rendering uses native `<text>` elements with the font's
     * family name resolved via FreeType. Viewers that don't have
     * the requested font fall back to `sans-serif`. (Path-embedded
     * glyphs for archival-perfect output are planned for a future
     * release.)
     */
    public function renderSvg(): string {}

    /**
     * Render to an SVG fragment: a single `<g class="fastchart">…</g>`
     * group with no outer `<svg>` or XML prolog. Intended for
     * stitching multiple charts into one caller-managed SVG document.
     * The caller owns the outer viewport / coordinate space.
     *
     * Available on every concrete `Chart` subclass — same coverage as
     * `renderSvg()`.
     */
    public function drawSvgFragment(): string {}
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
     * 127 is fully transparent and 0 is fully opaque — the same
     * convention `imagecolorallocatealpha()` uses). Default 64.
     * Stacked areas are always opaque.
     */
    public function setFillOpacity(int $alpha): static {}

}

final class BarChart extends Chart
{
    /** Orientation for setOrientation(). */
    public const int BAR_VERTICAL   = 0;
    public const int BAR_HORIZONTAL = 1;

    public function setSeries(array $series): static {}
    public function setStacked(bool $stacked): static {}

    /**
     * Bar orientation. BAR_VERTICAL (default) draws traditional
     * vertical bars with categories along the X axis. BAR_HORIZONTAL
     * draws bars running left-to-right with categories along the Y
     * axis -- useful when category labels are long. All other bar
     * features (stacking, floating, per-point colors, value labels)
     * carry over with X/Y semantics swapped.
     */
    public function setOrientation(int $orientation): static {}

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

}

final class ScatterChart extends Chart
{
    public function setPoints(array $points): static {}
    public function setMarkerStyle(int $style): static {}
    public function setMarkerSize(int $size): static {}

    /**
     * Overlay a least-squares regression curve over the scatter
     * points. `$degree = 1` (default) is the linear fit; 2 or 3
     * fits a polynomial of that order. Higher degrees are rejected;
     * quartic / quintic fits over noisy scatter data overfit and
     * are numerically fragile. Pass `$enabled = false` (default) to
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

}

final class StockChart extends Chart
{
    /** Moving-average kind passed to addMovingAverage(). */
    const int MA_SMA = 0;
    const int MA_EMA = 1;
    const int MA_WMA = 2;

    public function setOhlcv(array $ohlcv): static {}

    /**
     * Add a single moving-average overlay over the close price.
     * `$period` is the window length in bars (>= 2). `$type` selects
     * the smoothing kind:
     *   - `MA_SMA` simple moving average (arithmetic mean of the last
     *     `$period` closes)
     *   - `MA_EMA` exponential moving average (alpha = 2 /
     *     (period + 1), seeded with the SMA of the first `$period`
     *     closes)
     *   - `MA_WMA` linear-weighted moving average (weights 1..period,
     *     so the most recent close has weight `period`).
     *
     * Up to 8 overlays per chart; further calls raise ValueError.
     * Mix freely: `addMovingAverage(20, MA_SMA)` plus
     * `addMovingAverage(12, MA_EMA)` is the typical setup.
     */
    public function addMovingAverage(int $period, int $type = StockChart::MA_SMA): static {}

    /**
     * Bulk shortcut for adding several SMA overlays. Equivalent to
     * clearing the overlay list and calling `addMovingAverage($p,
     * MA_SMA)` for each period in `$periods`.
     */
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
     *   - `STYLE_CANDLE` (default): filled body + high-low wick.
     *   - `STYLE_BAR`: vertical wick + left tick at open + right
     *     tick at close (classic Western HLC bar).
     *   - `STYLE_DIAMOND`: diamond at close + high-low wick.
     *   - `STYLE_I_CAP`: wick with horizontal caps at high and low.
     *   - `STYLE_HOLLOW`: outlined-only body for bullish bars,
     *     filled body for bearish bars (TradingView convention).
     *   - `STYLE_VOLUME`: body width scales with the bar's volume
     *     relative to the rolling-average volume (requires the OHLCV
     *     row to carry a volume column).
     *   - `STYLE_VECTOR`: six-color scheme based on (direction) ×
     *     (volume strength: climax / rising / neutral). Uses the
     *     same algorithm as the pinescript "Vector Candles"
     *     indicator: climax when volume >= 2× avg or
     *     volume × (high-low) >= the running max; rising when volume
     *     >= 1.5× avg; otherwise neutral. Requires a volume column.
     */
    public function setCandleStyle(int $style): static {}

    public function addIndicatorPane(string $name, array $values, ?array $opts = null): static {}

    /**
     * Wilder's Relative Strength Index. Oscillator in [0, 100]
     * computed from close-to-close differences over `$period` bars
     * (default 14): seeds avg gain / loss with the SMA of the first
     * window, then updates with the standard recurrence
     * `avg = (avg * (p-1) + cur) / p`. Conventional reference lines:
     * 30 (oversold) and 70 (overbought); the pane renders 50 by
     * default; lower it via setIndicatorPane opts if you prefer.
     *
     * Requires `setOhlcv()` to have been called first; the values
     * are computed at this call. A subsequent `setOhlcv()` does NOT
     * recompute. Re-add the indicator after replacing the data.
     */
    public function addRSI(int $period = 14): static {}

    /**
     * Momentum oscillator: `close[i] - close[i-period]`. Centered
     * on zero; positive values mean upward momentum over the
     * window. Default `$period` is 10. Requires `setOhlcv()` first
     * (see addRSI for the order constraint).
     */
    public function addMomentum(int $period = 10): static {}

    /**
     * Rate of change: `(close[i] / close[i-period] - 1) * 100`,
     * expressed as a percentage. Centered on zero; default
     * `$period` is 10. NaN when the comparison close was zero.
     * Requires `setOhlcv()` first.
     */
    public function addROC(int $period = 10): static {}

    /**
     * On-balance volume: cumulative running sum of signed volume.
     * Each bar contributes +volume if its close is above the
     * previous close, -volume if below, 0 otherwise. Bars without
     * a volume column contribute zero. Requires `setOhlcv()` first;
     * starts the cumulative at 0 by convention.
     */
    public function addOBV(): static {}

    /**
     * MACD (Moving Average Convergence Divergence): a separate
     * pane with three series: MACD line (EMA fast minus EMA slow),
     * signal line (EMA of MACD with `signal` period), and a
     * histogram of MACD minus signal coloured by sign.
     *
     * Defaults are the classical (12, 26, 9). Requires
     * `setOhlcv()` first; the values are computed at this call.
     * Each pane counts against the 3-pane indicator cap.
     */
    public function addMACD(int $fast = 12, int $slow = 26, int $signal = 9): static {}

    /**
     * Stochastic oscillator: a pane with two lines, %K and %D.
     *   - %K[i] = (close[i] − lowest_low(period)) /
     *             (highest_high(period) − lowest_low(period)) × 100
     *   - %D    = SMA(%K, smooth)
     *
     * Default (14, 3) matches the conventional fast / smooth setup.
     * Range pinned to [0, 100]; reference lines at 20 (oversold)
     * and 80 (overbought) by convention; the renderer draws no
     * reference; configure setIndicatorPane opts on the resulting
     * pane if you want them.
     */
    public function addStochastic(int $period = 14, int $smooth = 3): static {}

    /**
     * Bollinger Bands: three lines OVERLAID on the price pane.
     * The middle SMA(close, period), an upper band at +n·σ above,
     * and a lower band at −n·σ below. Default (20, 2.0). Doesn't
     * consume an indicator-pane slot; uses the price-overlay
     * budget (4 overlays).
     */
    public function addBollingerBands(int $period = 20, float $stddev = 2.0): static {}

    /**
     * Parabolic SAR (Wilder): a dot per bar overlaid on the price
     * pane, indicating the trailing stop level for the current
     * trend. `af_init` is the starting acceleration factor (default
     * 0.02), `af_max` the cap (default 0.2). Requires `setOhlcv()`
     * first and at least 3 candles. Uses the price-overlay budget.
     */
    public function addParabolicSAR(float $af_init = 0.02, float $af_max = 0.2): static {}

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

}

/**
 * Bubble chart. Each point is `[x, y, size]`, optionally
 * `[x, y, size, color]`. Size is a positive radius in pixels.
 */
final class BubbleChart extends Chart
{
    public function setPoints(array $points): static {}
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
     * Show the numeric value inside each cell. Default false.
     */
    public function setShowCellValues(bool $show, string $format = '%g'): static {}

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

}

/**
 * Polar plot: continuous angular coordinate (radar's parent shape).
 * Points are `[angle_deg, radius]` and connect into a polygon.
 */
final class PolarChart extends Chart
{
    /** setStyle(): line/area mode (default), connect points into a polygon. */
    const int STYLE_LINE = 0;
    /** setStyle(): rose mode, each point is an angular wedge from the centre. */
    const int STYLE_ROSE = 1;

    /**
     * Points (single series) or list of series with
     * `['data' => [[deg, r], ...], 'label' => 'name', 'color' => int]`.
     *
     * In `STYLE_ROSE`, each entry's angle is the wedge START and
     * the angular width runs to the NEXT entry's angle (or evenly
     * spaced when the series is uniformly distributed). Radius
     * controls wedge length.
     */
    public function setSeries(array $series): static {}

    /** Force the radial scale. 0 = auto. */
    public function setMaxRadius(float $max): static {}

    /** Fill the polygon area in the series color (translucent). */
    public function setFilled(bool $filled): static {}

    /**
     * Switch between line/area mode and rose (angular bar) mode.
     * `STYLE_LINE` (default) connects points into a polygon, optionally
     * filled. `STYLE_ROSE` renders each (angle, radius) as a filled
     * wedge, useful for histograms in polar coordinates (wind roses,
     * bearing distributions).
     */
    public function setStyle(int $style): static {}

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

}

/**
 * Treemap: rectangle packing where each cell's area is proportional
 * to its `value`. Useful for flattened-hierarchy weighted views:
 * revenue-by-product, log-volume-by-source, market-cap-by-ticker.
 *
 * The squarify algorithm (Bruls / Huijsen / van Wijk) optimises
 * cell aspect ratios so cells stay close to square. Items with
 * non-positive `value` are silently dropped at setItems().
 */
final class Treemap extends Chart
{
    /**
     * Cell list. Each entry is
     * `['label' => string?, 'value' => number, 'color' => int?]`.
     * `value` is required and must be > 0; non-positive entries are
     * dropped. `label` is optional and centred in the cell; cells
     * too small to fit the label leave it blank. `color` is a 24-bit
     * RGB; missing draws from the theme palette.
     */
    public function setItems(array $items): static {}

    /**
     * Toggle rendering of cell labels. Default true. Disable for
     * dense charts where labels clutter; colour carries the cell-
     * identity signal.
     */
    public function setShowLabels(bool $enabled): static {}

}

/**
 * Funnel chart: descending stacked horizontal trapezoids. Each
 * stage's width is proportional to its value relative to the
 * largest stage. Common use: conversion funnels, drop-off rates.
 */
final class Funnel extends Chart
{
    /**
     * Stages, top to bottom. Each entry is
     * `['label' => string?, 'value' => number, 'color' => int?]`.
     * `value` is required and must be > 0; non-positive entries are
     * silently dropped at setStages().
     */
    public function setStages(array $stages): static {}

    /* setShowValues(bool, string $format = '%g') is inherited from
     * Chart and toggles the value labels rendered next to each
     * stage. The funnel default is to show values. */

}

/**
 * Waterfall chart: bar series with rising / falling / total
 * semantics. Useful for income statements, budget breakdowns,
 * step-change attribution. Each bar starts at the prior cumulative
 * and runs to cumulative + value, except `'kind' => 'total'` which
 * renders an absolute bar from zero.
 */
final class Waterfall extends Chart
{
    /**
     * Bars, in display order. Each entry is
     * `['label' => string, 'value' => number, 'kind' => 'delta'|'total']`.
     * `kind` defaults to `'delta'`. Delta bars carry signed values;
     * positive renders in the rise colour, negative in the fall
     * colour. Total bars use the total colour and reset the running
     * cumulative to their value.
     */
    public function setBars(array $bars): static {}

    public function setRiseColor(int $rgb): static {}
    public function setFallColor(int $rgb): static {}
    public function setTotalColor(int $rgb): static {}

}

/**
 * Discrete heatmap: a 2D grid of cells coloured by value. Distinct
 * from `ContourChart` (which interpolates isolines through the same
 * input shape); heatmap colours each cell directly from a low/high
 * ramp, optionally writing the cell value inside.
 */
final class Heatmap extends Chart
{
    /** 2D array of numeric values. Rows of equal length expected. */
    public function setGrid(array $grid): static {}

    /**
     * Color ramp for the cell-value→colour interpolation. Both
     * arguments are 24-bit RGB. Defaults to a cool-blue → warm-red
     * ramp.
     */
    public function setColorRamp(int $low, int $high): static {}

    /* setShowValues(bool, string $format = '%g') is inherited from
     * Chart; it toggles the per-cell value rendering AND sets the
     * printf format used for it. */

}

/**
 * Linear meter: bar-shaped gauge. Same zone / value / format
 * vocabulary as `GaugeChart`, rotated to a horizontal or vertical
 * bar. Useful for compact status / capacity readouts where a
 * round gauge is too tall.
 */
final class LinearMeter extends Chart
{
    const int METER_HORIZONTAL = 0;
    const int METER_VERTICAL   = 1;

    /** Set the meter's data range. min must be < max. */
    public function setRange(float $min, float $max): static {}

    /** Set the current value. Clamped to [min, max] at draw time. */
    public function setValue(float $value): static {}

    /** Choose horizontal or vertical orientation. Default horizontal. */
    public function setOrientation(int $orientation): static {}

    /**
     * Coloured zones along the bar. Each entry:
     * `['from' => number, 'to' => number, 'color' => int?]`. Up to
     * 8 zones; out-of-range or empty zones are skipped.
     */
    public function setZones(array $zones): static {}

    /**
     * Printf format for the min / max / current-value labels.
     * Default `%.0f`. Same validation rules as setYAxisLabelFormat.
     */
    public function setValueFormat(string $format): static {}

}

/**
 * Symbol family: 1D barcodes and 2D matrix codes (QR). Render-only
 * surface — Symbol classes do not accept a caller-supplied canvas.
 * Use the render*() / renderToFile() helpers to materialise the symbol.
 *
 * Symbol does not extend `Chart`; the two hierarchies share no state
 * (axes, palettes, plot rect, font cache do not apply to symbologies).
 */
abstract class Symbol
{
    /**
     * SVG text emission mode for setSvgTextMode(). Mirrors
     * Chart::SVG_TEXT_NATIVE / Chart::SVG_TEXT_PATHS. PATHS is the
     * default and matches the Chart-side semantics.
     */
    public const int SVG_TEXT_NATIVE = 0;
    public const int SVG_TEXT_PATHS  = 1;

    /**
     * Logical canvas size in pixels. Both arguments must be positive
     * and ≤ 65535. Setting size 0 is rejected; if you want the
     * class default, simply do not call `setSize()`. Physical
     * dimensions scale with `setDpi()` and are capped at 16384px /
     * 64M pixels — see Chart::setDpi() docs for the cap policy.
     */
    public function setSize(int $width, int $height): static {}

    /**
     * Payload to encode. Embedded NUL bytes are rejected with
     * `\ValueError` (gd's text path truncates at NUL; we reject
     * rather than encode a different string than the user passed in).
     * Empty strings are also rejected.
     */
    public function setData(string $data): static {}

    /**
     * Quiet-zone (whitespace) margin around the symbol. Units are
     * per-class: `Code128` measures it in pixels (default 10× the
     * narrowest bar), `QrCode` measures it in modules (default 4 per
     * the QR spec). Pass -1 to revert to the class default; any
     * other negative value is rejected.
     */
    public function setQuietZone(int $units): static {}

    /** Foreground colour as 24-bit RGB (0..0xFFFFFF). Default 0x000000. */
    public function setForeground(int $rgb): static {}

    /** Background colour as 24-bit RGB (0..0xFFFFFF). Default 0xFFFFFF. */
    public function setBackground(int $rgb): static {}

    /**
     * Make the background transparent in the encoded output. Honoured
     * by PNG and WebP; JPEG falls through to the background colour
     * because the format can't carry alpha.
     */
    public function setTransparentBackground(bool $enabled): static {}

    /**
     * Output DPI, between 24 and 1200. Default 96. Matches
     * `Chart::setDpi()` semantics: logical canvas size stays the
     * same, physical canvas grows for crisper rendering.
     */
    public function setDpi(int $dpi): static {}

    /**
     * SVG text-emission mode. See Chart::setSvgTextMode().
     */
    public function setSvgTextMode(int $mode): static {}

    /**
     * JPEG encode quality 1..100; default 88. See
     * Chart::setJpegQuality().
     */
    public function setJpegQuality(int $quality): static {}

    public function renderPng(): string {}
    public function renderJpeg(int $quality = 0): string {}
    public function renderWebp(int $quality = 90): string {}

    /**
     * Render to an SVG document. Returns the full markup including
     * the `<?xml ...>` prolog and `<svg>` root. The viewport matches
     * the logical canvas size (per-class default applies when
     * `setSize()` was not called). DPI does not scale the viewport;
     * SVG is vector and scales infinitely.
     */
    public function renderSvg(): string {}

    /**
     * Render to an SVG fragment: a single
     * `<g class="fastchart-symbol">…</g>` group with no outer `<svg>`
     * or XML prolog. Intended for stitching multiple charts /
     * symbols into one caller-managed SVG document.
     */
    public function drawSvgFragment(): string {}

    /**
     * Render and write to `$path`. Format inferred from extension;
     * supports .png / .jpg / .jpeg / .webp / .svg. `$quality`
     * applies to JPEG / WebP and is ignored for PNG and SVG. Honours
     * `open_basedir`. Returns bytes written. `.gif` / `.avif` raise a
     * clear "dropped in v1.0" Error.
     */
    public function renderToFile(string $path, int $quality = 90): int {}
}

/**
 * Abstract 1D linear-barcode base. Concrete subclass: `Code128`.
 * Other 1D symbologies (Code 39, EAN/UPC, ITF, Codabar, ...) will
 * subclass this base when implemented; each declares its own
 * text-rendering / checksum setters because the supported character
 * set and human-readable layout differ per symbology.
 */
abstract class Barcode extends Symbol
{
}

/**
 * Code 128: high-density alphanumeric 1D barcode with three subsets
 * (A: control + uppercase, B: full ASCII printable, C: digit pairs).
 * The encoder auto-switches between subsets to minimise encoded
 * length; mod-103 checksum is appended automatically. ISO/IEC 15417.
 *
 * Default canvas size when `setSize()` is not called: 300x80.
 */
final class Code128 extends Barcode
{
    /**
     * Render the human-readable payload below the bar pattern.
     * Default false. The font follows the same auto-detection chain
     * as Chart's label font.
     */
    public function setShowText(bool $enabled): static {}
}

/**
 * QR Code (ISO/IEC 18004). Two-dimensional matrix code with four
 * error-correction levels. Encoder is the vendored nayuki C library
 * (versions 1..40; auto-pick within the requested range).
 *
 * Default canvas size when `setSize()` is not called: 300x300.
 * Default ECC: M (~15% recovery). Default version range: 1..40 (the
 * encoder picks the smallest version that fits the data).
 *
 * **Input encoding:** `setData()` payloads must be valid UTF-8 (or
 * the ASCII subset thereof). The underlying encoder treats the
 * string as UTF-8 text and selects the most compact QR mode that
 * fits — numeric, alphanumeric, or byte. Invalid UTF-8 byte
 * sequences are not rejected by `setData()` (which only forbids
 * embedded NULs) but produce QR symbols that decode back to garbage
 * or unspecified bytes. If you need to encode arbitrary binary data,
 * base64-encode upstream and decode after scan.
 */
final class QrCode extends Symbol
{
    /** ECC level L: ~7% recovery. */
    const int ECC_L = 0;
    /** ECC level M: ~15% recovery (default). */
    const int ECC_M = 1;
    /** ECC level Q: ~25% recovery. */
    const int ECC_Q = 2;
    /** ECC level H: ~30% recovery. */
    const int ECC_H = 3;

    /**
     * Set the MINIMUM error-correction level. Pass one of the ECC_*
     * class constants. The encoder may raise the effective ECC level
     * if there is spare codeword space at the chosen version (the
     * "boost ECC" feature of the underlying nayuki encoder), which
     * means the resulting symbol has at LEAST the requested error-
     * correction tolerance and possibly more. The version range
     * (`setMinVersion()` / `setMaxVersion()`) is honoured strictly;
     * only the ECC level may be boosted within that range.
     */
    public function setEcc(int $level): static {}

    /**
     * Floor of the version range the encoder will consider. Range
     * 1..40. Defaults to 1.
     */
    public function setMinVersion(int $version): static {}

    /**
     * Ceiling of the version range the encoder will consider. Range
     * 1..40. Defaults to 40. The encoder fails the render if the
     * data + ECC level cannot fit at any version ≤ this ceiling.
     */
    public function setMaxVersion(int $version): static {}
}
