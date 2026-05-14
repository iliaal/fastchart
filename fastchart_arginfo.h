/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: d2ce65cfd5ebe27d69f0c8662c361a77b1c79127 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_FastChart_Chart___construct, 0, 0, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, width, IS_LONG, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, height, IS_LONG, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_version, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setSize, 0, 2, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, width, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, height, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setTitle, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, title, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setTheme, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, theme, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setBackgroundColor, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, rgb, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_setPlotBackgroundColor arginfo_class_FastChart_Chart_setBackgroundColor

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setSeriesColors, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, colors, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setFontPath, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setFontSize, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, size, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setCategoryLabels, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, labels, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setLegendPosition, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, position, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setYAxisScale, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, scale, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setStrict, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, strict, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_setXAxisTitle arginfo_class_FastChart_Chart_setTitle

#define arginfo_class_FastChart_Chart_setYAxisTitle arginfo_class_FastChart_Chart_setTitle

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setXAxisLabelAngle, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, degrees, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setYAxisRange, 0, 0, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, min, IS_DOUBLE, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, max, IS_DOUBLE, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, interval, IS_DOUBLE, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setSecondaryYAxis, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, enabled, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_addHorizontalLine, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, label, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, color, IS_LONG, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_addVerticalLine, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, position, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, label, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, color, IS_LONG, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_addIconAt, 0, 3, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, maxWidth, IS_LONG, 0, "-1")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, maxHeight, IS_LONG, 0, "-1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_addHorizontalBand, 0, 3, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, low, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, high, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, color, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, alpha, IS_LONG, 0, "64")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, label, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_addVerticalBand arginfo_class_FastChart_Chart_addHorizontalBand

#define arginfo_class_FastChart_Chart_setAxisColor arginfo_class_FastChart_Chart_setBackgroundColor

#define arginfo_class_FastChart_Chart_setGridColor arginfo_class_FastChart_Chart_setBackgroundColor

#define arginfo_class_FastChart_Chart_setBorderColor arginfo_class_FastChart_Chart_setBackgroundColor

#define arginfo_class_FastChart_Chart_setTextColor arginfo_class_FastChart_Chart_setBackgroundColor

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setTitleFont, 0, 0, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, path, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, size, IS_DOUBLE, 1, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_setAxisFont arginfo_class_FastChart_Chart_setTitleFont

#define arginfo_class_FastChart_Chart_setLabelFont arginfo_class_FastChart_Chart_setTitleFont

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setShowValues, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, show, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, format, IS_STRING, 0, "\'%g\'")
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_setTransparentBackground arginfo_class_FastChart_Chart_setSecondaryYAxis

#define arginfo_class_FastChart_Chart_setBackgroundImage arginfo_class_FastChart_Chart_setFontPath

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setLineInterpolation, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, mode, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setPlotRect, 0, 4, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, x0, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y0, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, x1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y1, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setBorderSides, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, sides, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_addOverlaySeries, 0, 2, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, type, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, values, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, opts, IS_ARRAY, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setXAxisVisible, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, visible, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_setYAxisVisible arginfo_class_FastChart_Chart_setXAxisVisible

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setYAxisLabelFormat, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, format, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_setXAxisLabelFormat arginfo_class_FastChart_Chart_setYAxisLabelFormat

#define arginfo_class_FastChart_Chart_setTickMode arginfo_class_FastChart_Chart_setLineInterpolation

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setBarWidth, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, percent, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_setEdgeColor arginfo_class_FastChart_Chart_setBackgroundColor

#define arginfo_class_FastChart_Chart_setZeroShelf arginfo_class_FastChart_Chart_setSecondaryYAxis

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setXLabelStride, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, stride, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_setSecondaryYAxisTitle arginfo_class_FastChart_Chart_setTitle

#define arginfo_class_FastChart_Chart_setThumbnailMode arginfo_class_FastChart_Chart_setSecondaryYAxis

#define arginfo_class_FastChart_Chart_setTitleColor arginfo_class_FastChart_Chart_setBackgroundColor

#define arginfo_class_FastChart_Chart_setAxisLabelColor arginfo_class_FastChart_Chart_setBackgroundColor

#define arginfo_class_FastChart_Chart_setAxisTitleColor arginfo_class_FastChart_Chart_setBackgroundColor

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setColorRamp, 0, 2, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, low, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, high, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_addTextAnnotation, 0, 3, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, text, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, color, IS_LONG, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setLineStyle, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, style, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setGradientFill, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, from, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, to, IS_LONG, 0, "-1")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, direction, IS_LONG, 0, "FastChart\\Chart::GRADIENT_VERTICAL")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setDropShadow, 0, 2, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, offsetX, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, offsetY, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, color, IS_LONG, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setShadowAlpha, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, alpha, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setDateAxisStride, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, unit, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, every, IS_LONG, 0, "1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_setDpi, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, dpi, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_FastChart_Chart_draw, 0, 1, GdImage, 0)
	ZEND_ARG_OBJ_INFO(0, canvas, GdImage, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_renderPng arginfo_class_FastChart_Chart_version

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_renderJpeg, 0, 0, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, quality, IS_LONG, 0, "90")
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_renderWebp arginfo_class_FastChart_Chart_renderJpeg

#define arginfo_class_FastChart_Chart_renderGif arginfo_class_FastChart_Chart_version

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_renderAvif, 0, 0, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, quality, IS_LONG, 0, "60")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_renderToFile, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, quality, IS_LONG, 0, "90")
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Chart_renderSvg arginfo_class_FastChart_Chart_version

#define arginfo_class_FastChart_Chart_drawSvgFragment arginfo_class_FastChart_Chart_version

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_LineChart_setSeries, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, series, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_LineChart_setMarkerStyle arginfo_class_FastChart_Chart_setLineStyle

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_LineChart_setMarkerSize, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, size, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_LineChart_setErrorBars, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, errors, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_LineChart_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_AreaChart_setSeries arginfo_class_FastChart_LineChart_setSeries

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_AreaChart_setStacked, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, stacked, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_AreaChart_setFillOpacity arginfo_class_FastChart_Chart_setShadowAlpha

#define arginfo_class_FastChart_AreaChart_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_BarChart_setSeries arginfo_class_FastChart_LineChart_setSeries

#define arginfo_class_FastChart_BarChart_setStacked arginfo_class_FastChart_AreaChart_setStacked

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_BarChart_setOrientation, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, orientation, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_BarChart_setStackMode arginfo_class_FastChart_Chart_setLineInterpolation

#define arginfo_class_FastChart_BarChart_setFloating arginfo_class_FastChart_Chart_setSecondaryYAxis

#define arginfo_class_FastChart_BarChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_PieChart_setSlices, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, slices, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_PieChart_setDonutHoleRatio, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, ratio, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_PieChart_setExplode, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, offsets, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_PieChart_setSliceLabelPosition arginfo_class_FastChart_Chart_setLegendPosition

#define arginfo_class_FastChart_PieChart_setSliceLabelFormat arginfo_class_FastChart_Chart_setYAxisLabelFormat

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_PieChart_setOtherThreshold, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, percent, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, label, IS_STRING, 0, "\'Other\'")
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_PieChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_ScatterChart_setPoints, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, points, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_ScatterChart_setMarkerStyle arginfo_class_FastChart_Chart_setLineStyle

#define arginfo_class_FastChart_ScatterChart_setMarkerSize arginfo_class_FastChart_LineChart_setMarkerSize

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_ScatterChart_setTrendLine, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, enabled, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, color, IS_LONG, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, degree, IS_LONG, 0, "1")
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_ScatterChart_setErrorBars arginfo_class_FastChart_LineChart_setErrorBars

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_ScatterChart_getImageMap, 0, 0, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, name, IS_STRING, 0, "\'fastchart\'")
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_ScatterChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_setOhlcv, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, ohlcv, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_addMovingAverage, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, period, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, type, IS_LONG, 0, "FastChart\\StockChart::MA_SMA")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_setMovingAverages, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, periods, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_StockChart_setVolumePane arginfo_class_FastChart_Chart_setSecondaryYAxis

#define arginfo_class_FastChart_StockChart_setVolumeColors arginfo_class_FastChart_Chart_setSeriesColors

#define arginfo_class_FastChart_StockChart_setCandleStyle arginfo_class_FastChart_Chart_setLineStyle

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_addIndicatorPane, 0, 2, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, values, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, opts, IS_ARRAY, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_addRSI, 0, 0, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, period, IS_LONG, 0, "14")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_addMomentum, 0, 0, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, period, IS_LONG, 0, "10")
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_StockChart_addROC arginfo_class_FastChart_StockChart_addMomentum

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_addOBV, 0, 0, IS_STATIC, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_addMACD, 0, 0, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, fast, IS_LONG, 0, "12")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, slow, IS_LONG, 0, "26")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, signal, IS_LONG, 0, "9")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_addStochastic, 0, 0, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, period, IS_LONG, 0, "14")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, smooth, IS_LONG, 0, "3")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_addBollingerBands, 0, 0, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, period, IS_LONG, 0, "20")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, stddev, IS_DOUBLE, 0, "2.0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_addParabolicSAR, 0, 0, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, af_init, IS_DOUBLE, 0, "0.02")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, af_max, IS_DOUBLE, 0, "0.2")
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_StockChart_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_RadarChart_setSeries arginfo_class_FastChart_LineChart_setSeries

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_RadarChart_setMaxValue, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, max, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_RadarChart_setFilled, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, filled, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_RadarChart_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_BubbleChart_setPoints arginfo_class_FastChart_ScatterChart_setPoints

#define arginfo_class_FastChart_BubbleChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_SurfaceChart_setGrid, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, grid, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_SurfaceChart_setShowCellValues arginfo_class_FastChart_Chart_setShowValues

#define arginfo_class_FastChart_SurfaceChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_GaugeChart_setValue, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_GaugeChart_setRange, 0, 2, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, min, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, max, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_GaugeChart_setZones, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, zones, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_GaugeChart_setValueFormat arginfo_class_FastChart_Chart_setYAxisLabelFormat

#define arginfo_class_FastChart_GaugeChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_GanttChart_setTasks, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, tasks, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_GanttChart_setTimeRange, 0, 0, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, start, IS_LONG, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, end, IS_LONG, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_GanttChart_setShowTaskLabels, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, show, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_GanttChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_BoxPlot_setBoxes, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, boxes, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_BoxPlot_setBoxWidth arginfo_class_FastChart_Chart_setBarWidth

#define arginfo_class_FastChart_BoxPlot_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_PolarChart_setSeries arginfo_class_FastChart_LineChart_setSeries

#define arginfo_class_FastChart_PolarChart_setMaxRadius arginfo_class_FastChart_RadarChart_setMaxValue

#define arginfo_class_FastChart_PolarChart_setFilled arginfo_class_FastChart_RadarChart_setFilled

#define arginfo_class_FastChart_PolarChart_setStyle arginfo_class_FastChart_Chart_setLineStyle

#define arginfo_class_FastChart_PolarChart_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_ContourChart_setGrid arginfo_class_FastChart_SurfaceChart_setGrid

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_ContourChart_setLevels, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, levels, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_ContourChart_setFilled arginfo_class_FastChart_RadarChart_setFilled

#define arginfo_class_FastChart_ContourChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Treemap_setItems, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, items, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Treemap_setShowLabels arginfo_class_FastChart_Chart_setSecondaryYAxis

#define arginfo_class_FastChart_Treemap_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Funnel_setStages, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, stages, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Funnel_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Waterfall_setBars, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, bars, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Waterfall_setRiseColor arginfo_class_FastChart_Chart_setBackgroundColor

#define arginfo_class_FastChart_Waterfall_setFallColor arginfo_class_FastChart_Chart_setBackgroundColor

#define arginfo_class_FastChart_Waterfall_setTotalColor arginfo_class_FastChart_Chart_setBackgroundColor

#define arginfo_class_FastChart_Waterfall_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_Heatmap_setGrid arginfo_class_FastChart_SurfaceChart_setGrid

#define arginfo_class_FastChart_Heatmap_setColorRamp arginfo_class_FastChart_Chart_setColorRamp

#define arginfo_class_FastChart_Heatmap_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_LinearMeter_setRange arginfo_class_FastChart_GaugeChart_setRange

#define arginfo_class_FastChart_LinearMeter_setValue arginfo_class_FastChart_GaugeChart_setValue

#define arginfo_class_FastChart_LinearMeter_setOrientation arginfo_class_FastChart_BarChart_setOrientation

#define arginfo_class_FastChart_LinearMeter_setZones arginfo_class_FastChart_GaugeChart_setZones

#define arginfo_class_FastChart_LinearMeter_setValueFormat arginfo_class_FastChart_Chart_setYAxisLabelFormat

#define arginfo_class_FastChart_LinearMeter_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_Symbol_setSize arginfo_class_FastChart_Chart_setSize

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Symbol_setData, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Symbol_setQuietZone, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, units, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_Symbol_setForeground arginfo_class_FastChart_Chart_setBackgroundColor

#define arginfo_class_FastChart_Symbol_setBackground arginfo_class_FastChart_Chart_setBackgroundColor

#define arginfo_class_FastChart_Symbol_setTransparentBackground arginfo_class_FastChart_Chart_setSecondaryYAxis

#define arginfo_class_FastChart_Symbol_setDpi arginfo_class_FastChart_Chart_setDpi

#define arginfo_class_FastChart_Symbol_renderPng arginfo_class_FastChart_Chart_version

#define arginfo_class_FastChart_Symbol_renderJpeg arginfo_class_FastChart_Chart_renderJpeg

#define arginfo_class_FastChart_Symbol_renderWebp arginfo_class_FastChart_Chart_renderJpeg

#define arginfo_class_FastChart_Symbol_renderGif arginfo_class_FastChart_Chart_version

#define arginfo_class_FastChart_Symbol_renderAvif arginfo_class_FastChart_Chart_renderAvif

#define arginfo_class_FastChart_Symbol_renderSvg arginfo_class_FastChart_Chart_version

#define arginfo_class_FastChart_Symbol_drawSvgFragment arginfo_class_FastChart_Chart_version

#define arginfo_class_FastChart_Symbol_renderToFile arginfo_class_FastChart_Chart_renderToFile

#define arginfo_class_FastChart_Code128_setShowText arginfo_class_FastChart_Chart_setSecondaryYAxis

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_QrCode_setEcc, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, level, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_QrCode_setMinVersion, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, version, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_QrCode_setMaxVersion arginfo_class_FastChart_QrCode_setMinVersion

ZEND_METHOD(FastChart_Chart, __construct);
ZEND_METHOD(FastChart_Chart, version);
ZEND_METHOD(FastChart_Chart, setSize);
ZEND_METHOD(FastChart_Chart, setTitle);
ZEND_METHOD(FastChart_Chart, setTheme);
ZEND_METHOD(FastChart_Chart, setBackgroundColor);
ZEND_METHOD(FastChart_Chart, setPlotBackgroundColor);
ZEND_METHOD(FastChart_Chart, setSeriesColors);
ZEND_METHOD(FastChart_Chart, setFontPath);
ZEND_METHOD(FastChart_Chart, setFontSize);
ZEND_METHOD(FastChart_Chart, setCategoryLabels);
ZEND_METHOD(FastChart_Chart, setLegendPosition);
ZEND_METHOD(FastChart_Chart, setYAxisScale);
ZEND_METHOD(FastChart_Chart, setStrict);
ZEND_METHOD(FastChart_Chart, setXAxisTitle);
ZEND_METHOD(FastChart_Chart, setYAxisTitle);
ZEND_METHOD(FastChart_Chart, setXAxisLabelAngle);
ZEND_METHOD(FastChart_Chart, setYAxisRange);
ZEND_METHOD(FastChart_Chart, setSecondaryYAxis);
ZEND_METHOD(FastChart_Chart, addHorizontalLine);
ZEND_METHOD(FastChart_Chart, addVerticalLine);
ZEND_METHOD(FastChart_Chart, addIconAt);
ZEND_METHOD(FastChart_Chart, addHorizontalBand);
ZEND_METHOD(FastChart_Chart, addVerticalBand);
ZEND_METHOD(FastChart_Chart, setAxisColor);
ZEND_METHOD(FastChart_Chart, setGridColor);
ZEND_METHOD(FastChart_Chart, setBorderColor);
ZEND_METHOD(FastChart_Chart, setTextColor);
ZEND_METHOD(FastChart_Chart, setTitleFont);
ZEND_METHOD(FastChart_Chart, setAxisFont);
ZEND_METHOD(FastChart_Chart, setLabelFont);
ZEND_METHOD(FastChart_Chart, setShowValues);
ZEND_METHOD(FastChart_Chart, setTransparentBackground);
ZEND_METHOD(FastChart_Chart, setBackgroundImage);
ZEND_METHOD(FastChart_Chart, setLineInterpolation);
ZEND_METHOD(FastChart_Chart, setPlotRect);
ZEND_METHOD(FastChart_Chart, setBorderSides);
ZEND_METHOD(FastChart_Chart, addOverlaySeries);
ZEND_METHOD(FastChart_Chart, setXAxisVisible);
ZEND_METHOD(FastChart_Chart, setYAxisVisible);
ZEND_METHOD(FastChart_Chart, setYAxisLabelFormat);
ZEND_METHOD(FastChart_Chart, setXAxisLabelFormat);
ZEND_METHOD(FastChart_Chart, setTickMode);
ZEND_METHOD(FastChart_Chart, setBarWidth);
ZEND_METHOD(FastChart_Chart, setEdgeColor);
ZEND_METHOD(FastChart_Chart, setZeroShelf);
ZEND_METHOD(FastChart_Chart, setXLabelStride);
ZEND_METHOD(FastChart_Chart, setSecondaryYAxisTitle);
ZEND_METHOD(FastChart_Chart, setThumbnailMode);
ZEND_METHOD(FastChart_Chart, setTitleColor);
ZEND_METHOD(FastChart_Chart, setAxisLabelColor);
ZEND_METHOD(FastChart_Chart, setAxisTitleColor);
ZEND_METHOD(FastChart_Chart, setColorRamp);
ZEND_METHOD(FastChart_Chart, addTextAnnotation);
ZEND_METHOD(FastChart_Chart, setLineStyle);
ZEND_METHOD(FastChart_Chart, setGradientFill);
ZEND_METHOD(FastChart_Chart, setDropShadow);
ZEND_METHOD(FastChart_Chart, setShadowAlpha);
ZEND_METHOD(FastChart_Chart, setDateAxisStride);
ZEND_METHOD(FastChart_Chart, setDpi);
ZEND_METHOD(FastChart_Chart, renderPng);
ZEND_METHOD(FastChart_Chart, renderJpeg);
ZEND_METHOD(FastChart_Chart, renderWebp);
ZEND_METHOD(FastChart_Chart, renderGif);
ZEND_METHOD(FastChart_Chart, renderAvif);
ZEND_METHOD(FastChart_Chart, renderToFile);
ZEND_METHOD(FastChart_Chart, renderSvg);
ZEND_METHOD(FastChart_Chart, drawSvgFragment);
ZEND_METHOD(FastChart_LineChart, setSeries);
ZEND_METHOD(FastChart_LineChart, setMarkerStyle);
ZEND_METHOD(FastChart_LineChart, setMarkerSize);
ZEND_METHOD(FastChart_LineChart, setErrorBars);
ZEND_METHOD(FastChart_LineChart, draw);
ZEND_METHOD(FastChart_AreaChart, setSeries);
ZEND_METHOD(FastChart_AreaChart, setStacked);
ZEND_METHOD(FastChart_AreaChart, setFillOpacity);
ZEND_METHOD(FastChart_AreaChart, draw);
ZEND_METHOD(FastChart_BarChart, setSeries);
ZEND_METHOD(FastChart_BarChart, setStacked);
ZEND_METHOD(FastChart_BarChart, setOrientation);
ZEND_METHOD(FastChart_BarChart, setStackMode);
ZEND_METHOD(FastChart_BarChart, setFloating);
ZEND_METHOD(FastChart_BarChart, draw);
ZEND_METHOD(FastChart_PieChart, setSlices);
ZEND_METHOD(FastChart_PieChart, setDonutHoleRatio);
ZEND_METHOD(FastChart_PieChart, setExplode);
ZEND_METHOD(FastChart_PieChart, setSliceLabelPosition);
ZEND_METHOD(FastChart_PieChart, setSliceLabelFormat);
ZEND_METHOD(FastChart_PieChart, setOtherThreshold);
ZEND_METHOD(FastChart_PieChart, draw);
ZEND_METHOD(FastChart_ScatterChart, setPoints);
ZEND_METHOD(FastChart_ScatterChart, setMarkerStyle);
ZEND_METHOD(FastChart_ScatterChart, setMarkerSize);
ZEND_METHOD(FastChart_ScatterChart, setTrendLine);
ZEND_METHOD(FastChart_ScatterChart, setErrorBars);
ZEND_METHOD(FastChart_ScatterChart, getImageMap);
ZEND_METHOD(FastChart_ScatterChart, draw);
ZEND_METHOD(FastChart_StockChart, setOhlcv);
ZEND_METHOD(FastChart_StockChart, addMovingAverage);
ZEND_METHOD(FastChart_StockChart, setMovingAverages);
ZEND_METHOD(FastChart_StockChart, setVolumePane);
ZEND_METHOD(FastChart_StockChart, setVolumeColors);
ZEND_METHOD(FastChart_StockChart, setCandleStyle);
ZEND_METHOD(FastChart_StockChart, addIndicatorPane);
ZEND_METHOD(FastChart_StockChart, addRSI);
ZEND_METHOD(FastChart_StockChart, addMomentum);
ZEND_METHOD(FastChart_StockChart, addROC);
ZEND_METHOD(FastChart_StockChart, addOBV);
ZEND_METHOD(FastChart_StockChart, addMACD);
ZEND_METHOD(FastChart_StockChart, addStochastic);
ZEND_METHOD(FastChart_StockChart, addBollingerBands);
ZEND_METHOD(FastChart_StockChart, addParabolicSAR);
ZEND_METHOD(FastChart_StockChart, draw);
ZEND_METHOD(FastChart_RadarChart, setSeries);
ZEND_METHOD(FastChart_RadarChart, setMaxValue);
ZEND_METHOD(FastChart_RadarChart, setFilled);
ZEND_METHOD(FastChart_RadarChart, draw);
ZEND_METHOD(FastChart_BubbleChart, setPoints);
ZEND_METHOD(FastChart_BubbleChart, draw);
ZEND_METHOD(FastChart_SurfaceChart, setGrid);
ZEND_METHOD(FastChart_SurfaceChart, setShowCellValues);
ZEND_METHOD(FastChart_SurfaceChart, draw);
ZEND_METHOD(FastChart_GaugeChart, setValue);
ZEND_METHOD(FastChart_GaugeChart, setRange);
ZEND_METHOD(FastChart_GaugeChart, setZones);
ZEND_METHOD(FastChart_GaugeChart, setValueFormat);
ZEND_METHOD(FastChart_GaugeChart, draw);
ZEND_METHOD(FastChart_GanttChart, setTasks);
ZEND_METHOD(FastChart_GanttChart, setTimeRange);
ZEND_METHOD(FastChart_GanttChart, setShowTaskLabels);
ZEND_METHOD(FastChart_GanttChart, draw);
ZEND_METHOD(FastChart_BoxPlot, setBoxes);
ZEND_METHOD(FastChart_BoxPlot, setBoxWidth);
ZEND_METHOD(FastChart_BoxPlot, draw);
ZEND_METHOD(FastChart_PolarChart, setSeries);
ZEND_METHOD(FastChart_PolarChart, setMaxRadius);
ZEND_METHOD(FastChart_PolarChart, setFilled);
ZEND_METHOD(FastChart_PolarChart, setStyle);
ZEND_METHOD(FastChart_PolarChart, draw);
ZEND_METHOD(FastChart_ContourChart, setGrid);
ZEND_METHOD(FastChart_ContourChart, setLevels);
ZEND_METHOD(FastChart_ContourChart, setFilled);
ZEND_METHOD(FastChart_ContourChart, draw);
ZEND_METHOD(FastChart_Treemap, setItems);
ZEND_METHOD(FastChart_Treemap, setShowLabels);
ZEND_METHOD(FastChart_Treemap, draw);
ZEND_METHOD(FastChart_Funnel, setStages);
ZEND_METHOD(FastChart_Funnel, draw);
ZEND_METHOD(FastChart_Waterfall, setBars);
ZEND_METHOD(FastChart_Waterfall, setRiseColor);
ZEND_METHOD(FastChart_Waterfall, setFallColor);
ZEND_METHOD(FastChart_Waterfall, setTotalColor);
ZEND_METHOD(FastChart_Waterfall, draw);
ZEND_METHOD(FastChart_Heatmap, setGrid);
ZEND_METHOD(FastChart_Heatmap, setColorRamp);
ZEND_METHOD(FastChart_Heatmap, draw);
ZEND_METHOD(FastChart_LinearMeter, setRange);
ZEND_METHOD(FastChart_LinearMeter, setValue);
ZEND_METHOD(FastChart_LinearMeter, setOrientation);
ZEND_METHOD(FastChart_LinearMeter, setZones);
ZEND_METHOD(FastChart_LinearMeter, setValueFormat);
ZEND_METHOD(FastChart_LinearMeter, draw);
ZEND_METHOD(FastChart_Symbol, setSize);
ZEND_METHOD(FastChart_Symbol, setData);
ZEND_METHOD(FastChart_Symbol, setQuietZone);
ZEND_METHOD(FastChart_Symbol, setForeground);
ZEND_METHOD(FastChart_Symbol, setBackground);
ZEND_METHOD(FastChart_Symbol, setTransparentBackground);
ZEND_METHOD(FastChart_Symbol, setDpi);
ZEND_METHOD(FastChart_Symbol, renderPng);
ZEND_METHOD(FastChart_Symbol, renderJpeg);
ZEND_METHOD(FastChart_Symbol, renderWebp);
ZEND_METHOD(FastChart_Symbol, renderGif);
ZEND_METHOD(FastChart_Symbol, renderAvif);
ZEND_METHOD(FastChart_Symbol, renderSvg);
ZEND_METHOD(FastChart_Symbol, drawSvgFragment);
ZEND_METHOD(FastChart_Symbol, renderToFile);
ZEND_METHOD(FastChart_Code128, setShowText);
ZEND_METHOD(FastChart_QrCode, setEcc);
ZEND_METHOD(FastChart_QrCode, setMinVersion);
ZEND_METHOD(FastChart_QrCode, setMaxVersion);

static const zend_function_entry class_FastChart_Chart_methods[] = {
	ZEND_ME(FastChart_Chart, __construct, arginfo_class_FastChart_Chart___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, version, arginfo_class_FastChart_Chart_version, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(FastChart_Chart, setSize, arginfo_class_FastChart_Chart_setSize, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setTitle, arginfo_class_FastChart_Chart_setTitle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setTheme, arginfo_class_FastChart_Chart_setTheme, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setBackgroundColor, arginfo_class_FastChart_Chart_setBackgroundColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setPlotBackgroundColor, arginfo_class_FastChart_Chart_setPlotBackgroundColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setSeriesColors, arginfo_class_FastChart_Chart_setSeriesColors, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setFontPath, arginfo_class_FastChart_Chart_setFontPath, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setFontSize, arginfo_class_FastChart_Chart_setFontSize, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setCategoryLabels, arginfo_class_FastChart_Chart_setCategoryLabels, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setLegendPosition, arginfo_class_FastChart_Chart_setLegendPosition, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setYAxisScale, arginfo_class_FastChart_Chart_setYAxisScale, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setStrict, arginfo_class_FastChart_Chart_setStrict, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setXAxisTitle, arginfo_class_FastChart_Chart_setXAxisTitle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setYAxisTitle, arginfo_class_FastChart_Chart_setYAxisTitle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setXAxisLabelAngle, arginfo_class_FastChart_Chart_setXAxisLabelAngle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setYAxisRange, arginfo_class_FastChart_Chart_setYAxisRange, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setSecondaryYAxis, arginfo_class_FastChart_Chart_setSecondaryYAxis, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, addHorizontalLine, arginfo_class_FastChart_Chart_addHorizontalLine, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, addVerticalLine, arginfo_class_FastChart_Chart_addVerticalLine, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, addIconAt, arginfo_class_FastChart_Chart_addIconAt, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, addHorizontalBand, arginfo_class_FastChart_Chart_addHorizontalBand, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, addVerticalBand, arginfo_class_FastChart_Chart_addVerticalBand, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setAxisColor, arginfo_class_FastChart_Chart_setAxisColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setGridColor, arginfo_class_FastChart_Chart_setGridColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setBorderColor, arginfo_class_FastChart_Chart_setBorderColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setTextColor, arginfo_class_FastChart_Chart_setTextColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setTitleFont, arginfo_class_FastChart_Chart_setTitleFont, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setAxisFont, arginfo_class_FastChart_Chart_setAxisFont, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setLabelFont, arginfo_class_FastChart_Chart_setLabelFont, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setShowValues, arginfo_class_FastChart_Chart_setShowValues, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setTransparentBackground, arginfo_class_FastChart_Chart_setTransparentBackground, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setBackgroundImage, arginfo_class_FastChart_Chart_setBackgroundImage, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setLineInterpolation, arginfo_class_FastChart_Chart_setLineInterpolation, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setPlotRect, arginfo_class_FastChart_Chart_setPlotRect, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setBorderSides, arginfo_class_FastChart_Chart_setBorderSides, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, addOverlaySeries, arginfo_class_FastChart_Chart_addOverlaySeries, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setXAxisVisible, arginfo_class_FastChart_Chart_setXAxisVisible, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setYAxisVisible, arginfo_class_FastChart_Chart_setYAxisVisible, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setYAxisLabelFormat, arginfo_class_FastChart_Chart_setYAxisLabelFormat, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setXAxisLabelFormat, arginfo_class_FastChart_Chart_setXAxisLabelFormat, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setTickMode, arginfo_class_FastChart_Chart_setTickMode, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setBarWidth, arginfo_class_FastChart_Chart_setBarWidth, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setEdgeColor, arginfo_class_FastChart_Chart_setEdgeColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setZeroShelf, arginfo_class_FastChart_Chart_setZeroShelf, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setXLabelStride, arginfo_class_FastChart_Chart_setXLabelStride, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setSecondaryYAxisTitle, arginfo_class_FastChart_Chart_setSecondaryYAxisTitle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setThumbnailMode, arginfo_class_FastChart_Chart_setThumbnailMode, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setTitleColor, arginfo_class_FastChart_Chart_setTitleColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setAxisLabelColor, arginfo_class_FastChart_Chart_setAxisLabelColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setAxisTitleColor, arginfo_class_FastChart_Chart_setAxisTitleColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setColorRamp, arginfo_class_FastChart_Chart_setColorRamp, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, addTextAnnotation, arginfo_class_FastChart_Chart_addTextAnnotation, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setLineStyle, arginfo_class_FastChart_Chart_setLineStyle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setGradientFill, arginfo_class_FastChart_Chart_setGradientFill, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setDropShadow, arginfo_class_FastChart_Chart_setDropShadow, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setShadowAlpha, arginfo_class_FastChart_Chart_setShadowAlpha, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setDateAxisStride, arginfo_class_FastChart_Chart_setDateAxisStride, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setDpi, arginfo_class_FastChart_Chart_setDpi, ZEND_ACC_PUBLIC)
	ZEND_RAW_FENTRY("draw", NULL, arginfo_class_FastChart_Chart_draw, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT, NULL, NULL)
	ZEND_ME(FastChart_Chart, renderPng, arginfo_class_FastChart_Chart_renderPng, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, renderJpeg, arginfo_class_FastChart_Chart_renderJpeg, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, renderWebp, arginfo_class_FastChart_Chart_renderWebp, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, renderGif, arginfo_class_FastChart_Chart_renderGif, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, renderAvif, arginfo_class_FastChart_Chart_renderAvif, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, renderToFile, arginfo_class_FastChart_Chart_renderToFile, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, renderSvg, arginfo_class_FastChart_Chart_renderSvg, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, drawSvgFragment, arginfo_class_FastChart_Chart_drawSvgFragment, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_LineChart_methods[] = {
	ZEND_ME(FastChart_LineChart, setSeries, arginfo_class_FastChart_LineChart_setSeries, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LineChart, setMarkerStyle, arginfo_class_FastChart_LineChart_setMarkerStyle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LineChart, setMarkerSize, arginfo_class_FastChart_LineChart_setMarkerSize, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LineChart, setErrorBars, arginfo_class_FastChart_LineChart_setErrorBars, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LineChart, draw, arginfo_class_FastChart_LineChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_AreaChart_methods[] = {
	ZEND_ME(FastChart_AreaChart, setSeries, arginfo_class_FastChart_AreaChart_setSeries, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_AreaChart, setStacked, arginfo_class_FastChart_AreaChart_setStacked, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_AreaChart, setFillOpacity, arginfo_class_FastChart_AreaChart_setFillOpacity, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_AreaChart, draw, arginfo_class_FastChart_AreaChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_BarChart_methods[] = {
	ZEND_ME(FastChart_BarChart, setSeries, arginfo_class_FastChart_BarChart_setSeries, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_BarChart, setStacked, arginfo_class_FastChart_BarChart_setStacked, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_BarChart, setOrientation, arginfo_class_FastChart_BarChart_setOrientation, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_BarChart, setStackMode, arginfo_class_FastChart_BarChart_setStackMode, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_BarChart, setFloating, arginfo_class_FastChart_BarChart_setFloating, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_BarChart, draw, arginfo_class_FastChart_BarChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_PieChart_methods[] = {
	ZEND_ME(FastChart_PieChart, setSlices, arginfo_class_FastChart_PieChart_setSlices, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PieChart, setDonutHoleRatio, arginfo_class_FastChart_PieChart_setDonutHoleRatio, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PieChart, setExplode, arginfo_class_FastChart_PieChart_setExplode, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PieChart, setSliceLabelPosition, arginfo_class_FastChart_PieChart_setSliceLabelPosition, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PieChart, setSliceLabelFormat, arginfo_class_FastChart_PieChart_setSliceLabelFormat, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PieChart, setOtherThreshold, arginfo_class_FastChart_PieChart_setOtherThreshold, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PieChart, draw, arginfo_class_FastChart_PieChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_ScatterChart_methods[] = {
	ZEND_ME(FastChart_ScatterChart, setPoints, arginfo_class_FastChart_ScatterChart_setPoints, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_ScatterChart, setMarkerStyle, arginfo_class_FastChart_ScatterChart_setMarkerStyle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_ScatterChart, setMarkerSize, arginfo_class_FastChart_ScatterChart_setMarkerSize, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_ScatterChart, setTrendLine, arginfo_class_FastChart_ScatterChart_setTrendLine, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_ScatterChart, setErrorBars, arginfo_class_FastChart_ScatterChart_setErrorBars, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_ScatterChart, getImageMap, arginfo_class_FastChart_ScatterChart_getImageMap, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_ScatterChart, draw, arginfo_class_FastChart_ScatterChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_StockChart_methods[] = {
	ZEND_ME(FastChart_StockChart, setOhlcv, arginfo_class_FastChart_StockChart_setOhlcv, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, addMovingAverage, arginfo_class_FastChart_StockChart_addMovingAverage, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, setMovingAverages, arginfo_class_FastChart_StockChart_setMovingAverages, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, setVolumePane, arginfo_class_FastChart_StockChart_setVolumePane, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, setVolumeColors, arginfo_class_FastChart_StockChart_setVolumeColors, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, setCandleStyle, arginfo_class_FastChart_StockChart_setCandleStyle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, addIndicatorPane, arginfo_class_FastChart_StockChart_addIndicatorPane, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, addRSI, arginfo_class_FastChart_StockChart_addRSI, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, addMomentum, arginfo_class_FastChart_StockChart_addMomentum, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, addROC, arginfo_class_FastChart_StockChart_addROC, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, addOBV, arginfo_class_FastChart_StockChart_addOBV, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, addMACD, arginfo_class_FastChart_StockChart_addMACD, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, addStochastic, arginfo_class_FastChart_StockChart_addStochastic, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, addBollingerBands, arginfo_class_FastChart_StockChart_addBollingerBands, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, addParabolicSAR, arginfo_class_FastChart_StockChart_addParabolicSAR, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, draw, arginfo_class_FastChart_StockChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_RadarChart_methods[] = {
	ZEND_ME(FastChart_RadarChart, setSeries, arginfo_class_FastChart_RadarChart_setSeries, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_RadarChart, setMaxValue, arginfo_class_FastChart_RadarChart_setMaxValue, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_RadarChart, setFilled, arginfo_class_FastChart_RadarChart_setFilled, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_RadarChart, draw, arginfo_class_FastChart_RadarChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_BubbleChart_methods[] = {
	ZEND_ME(FastChart_BubbleChart, setPoints, arginfo_class_FastChart_BubbleChart_setPoints, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_BubbleChart, draw, arginfo_class_FastChart_BubbleChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_SurfaceChart_methods[] = {
	ZEND_ME(FastChart_SurfaceChart, setGrid, arginfo_class_FastChart_SurfaceChart_setGrid, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_SurfaceChart, setShowCellValues, arginfo_class_FastChart_SurfaceChart_setShowCellValues, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_SurfaceChart, draw, arginfo_class_FastChart_SurfaceChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_GaugeChart_methods[] = {
	ZEND_ME(FastChart_GaugeChart, setValue, arginfo_class_FastChart_GaugeChart_setValue, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_GaugeChart, setRange, arginfo_class_FastChart_GaugeChart_setRange, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_GaugeChart, setZones, arginfo_class_FastChart_GaugeChart_setZones, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_GaugeChart, setValueFormat, arginfo_class_FastChart_GaugeChart_setValueFormat, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_GaugeChart, draw, arginfo_class_FastChart_GaugeChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_GanttChart_methods[] = {
	ZEND_ME(FastChart_GanttChart, setTasks, arginfo_class_FastChart_GanttChart_setTasks, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_GanttChart, setTimeRange, arginfo_class_FastChart_GanttChart_setTimeRange, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_GanttChart, setShowTaskLabels, arginfo_class_FastChart_GanttChart_setShowTaskLabels, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_GanttChart, draw, arginfo_class_FastChart_GanttChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_BoxPlot_methods[] = {
	ZEND_ME(FastChart_BoxPlot, setBoxes, arginfo_class_FastChart_BoxPlot_setBoxes, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_BoxPlot, setBoxWidth, arginfo_class_FastChart_BoxPlot_setBoxWidth, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_BoxPlot, draw, arginfo_class_FastChart_BoxPlot_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_PolarChart_methods[] = {
	ZEND_ME(FastChart_PolarChart, setSeries, arginfo_class_FastChart_PolarChart_setSeries, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PolarChart, setMaxRadius, arginfo_class_FastChart_PolarChart_setMaxRadius, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PolarChart, setFilled, arginfo_class_FastChart_PolarChart_setFilled, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PolarChart, setStyle, arginfo_class_FastChart_PolarChart_setStyle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PolarChart, draw, arginfo_class_FastChart_PolarChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_ContourChart_methods[] = {
	ZEND_ME(FastChart_ContourChart, setGrid, arginfo_class_FastChart_ContourChart_setGrid, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_ContourChart, setLevels, arginfo_class_FastChart_ContourChart_setLevels, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_ContourChart, setFilled, arginfo_class_FastChart_ContourChart_setFilled, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_ContourChart, draw, arginfo_class_FastChart_ContourChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_Treemap_methods[] = {
	ZEND_ME(FastChart_Treemap, setItems, arginfo_class_FastChart_Treemap_setItems, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Treemap, setShowLabels, arginfo_class_FastChart_Treemap_setShowLabels, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Treemap, draw, arginfo_class_FastChart_Treemap_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_Funnel_methods[] = {
	ZEND_ME(FastChart_Funnel, setStages, arginfo_class_FastChart_Funnel_setStages, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Funnel, draw, arginfo_class_FastChart_Funnel_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_Waterfall_methods[] = {
	ZEND_ME(FastChart_Waterfall, setBars, arginfo_class_FastChart_Waterfall_setBars, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Waterfall, setRiseColor, arginfo_class_FastChart_Waterfall_setRiseColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Waterfall, setFallColor, arginfo_class_FastChart_Waterfall_setFallColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Waterfall, setTotalColor, arginfo_class_FastChart_Waterfall_setTotalColor, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Waterfall, draw, arginfo_class_FastChart_Waterfall_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_Heatmap_methods[] = {
	ZEND_ME(FastChart_Heatmap, setGrid, arginfo_class_FastChart_Heatmap_setGrid, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Heatmap, setColorRamp, arginfo_class_FastChart_Heatmap_setColorRamp, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Heatmap, draw, arginfo_class_FastChart_Heatmap_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_LinearMeter_methods[] = {
	ZEND_ME(FastChart_LinearMeter, setRange, arginfo_class_FastChart_LinearMeter_setRange, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LinearMeter, setValue, arginfo_class_FastChart_LinearMeter_setValue, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LinearMeter, setOrientation, arginfo_class_FastChart_LinearMeter_setOrientation, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LinearMeter, setZones, arginfo_class_FastChart_LinearMeter_setZones, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LinearMeter, setValueFormat, arginfo_class_FastChart_LinearMeter_setValueFormat, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LinearMeter, draw, arginfo_class_FastChart_LinearMeter_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_Symbol_methods[] = {
	ZEND_ME(FastChart_Symbol, setSize, arginfo_class_FastChart_Symbol_setSize, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, setData, arginfo_class_FastChart_Symbol_setData, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, setQuietZone, arginfo_class_FastChart_Symbol_setQuietZone, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, setForeground, arginfo_class_FastChart_Symbol_setForeground, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, setBackground, arginfo_class_FastChart_Symbol_setBackground, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, setTransparentBackground, arginfo_class_FastChart_Symbol_setTransparentBackground, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, setDpi, arginfo_class_FastChart_Symbol_setDpi, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, renderPng, arginfo_class_FastChart_Symbol_renderPng, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, renderJpeg, arginfo_class_FastChart_Symbol_renderJpeg, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, renderWebp, arginfo_class_FastChart_Symbol_renderWebp, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, renderGif, arginfo_class_FastChart_Symbol_renderGif, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, renderAvif, arginfo_class_FastChart_Symbol_renderAvif, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, renderSvg, arginfo_class_FastChart_Symbol_renderSvg, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, drawSvgFragment, arginfo_class_FastChart_Symbol_drawSvgFragment, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Symbol, renderToFile, arginfo_class_FastChart_Symbol_renderToFile, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_Code128_methods[] = {
	ZEND_ME(FastChart_Code128, setShowText, arginfo_class_FastChart_Code128_setShowText, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_QrCode_methods[] = {
	ZEND_ME(FastChart_QrCode, setEcc, arginfo_class_FastChart_QrCode_setEcc, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_QrCode, setMinVersion, arginfo_class_FastChart_QrCode_setMinVersion, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_QrCode, setMaxVersion, arginfo_class_FastChart_QrCode_setMaxVersion, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_FastChart_Chart(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "Chart", class_FastChart_Chart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, ZEND_ACC_ABSTRACT);

	zval const_THEME_LIGHT_value;
	ZVAL_LONG(&const_THEME_LIGHT_value, 0);
	zend_string *const_THEME_LIGHT_name = zend_string_init_interned("THEME_LIGHT", sizeof("THEME_LIGHT") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_THEME_LIGHT_name, &const_THEME_LIGHT_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_THEME_LIGHT_name);

	zval const_THEME_DARK_value;
	ZVAL_LONG(&const_THEME_DARK_value, 1);
	zend_string *const_THEME_DARK_name = zend_string_init_interned("THEME_DARK", sizeof("THEME_DARK") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_THEME_DARK_name, &const_THEME_DARK_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_THEME_DARK_name);

	zval const_MARKER_NONE_value;
	ZVAL_LONG(&const_MARKER_NONE_value, 0);
	zend_string *const_MARKER_NONE_name = zend_string_init_interned("MARKER_NONE", sizeof("MARKER_NONE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_MARKER_NONE_name, &const_MARKER_NONE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_MARKER_NONE_name);

	zval const_MARKER_CIRCLE_value;
	ZVAL_LONG(&const_MARKER_CIRCLE_value, 1);
	zend_string *const_MARKER_CIRCLE_name = zend_string_init_interned("MARKER_CIRCLE", sizeof("MARKER_CIRCLE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_MARKER_CIRCLE_name, &const_MARKER_CIRCLE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_MARKER_CIRCLE_name);

	zval const_MARKER_SQUARE_value;
	ZVAL_LONG(&const_MARKER_SQUARE_value, 2);
	zend_string *const_MARKER_SQUARE_name = zend_string_init_interned("MARKER_SQUARE", sizeof("MARKER_SQUARE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_MARKER_SQUARE_name, &const_MARKER_SQUARE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_MARKER_SQUARE_name);

	zval const_MARKER_DIAMOND_value;
	ZVAL_LONG(&const_MARKER_DIAMOND_value, 3);
	zend_string *const_MARKER_DIAMOND_name = zend_string_init_interned("MARKER_DIAMOND", sizeof("MARKER_DIAMOND") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_MARKER_DIAMOND_name, &const_MARKER_DIAMOND_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_MARKER_DIAMOND_name);

	zval const_MARKER_CROSS_value;
	ZVAL_LONG(&const_MARKER_CROSS_value, 4);
	zend_string *const_MARKER_CROSS_name = zend_string_init_interned("MARKER_CROSS", sizeof("MARKER_CROSS") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_MARKER_CROSS_name, &const_MARKER_CROSS_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_MARKER_CROSS_name);

	zval const_MARKER_PLUS_value;
	ZVAL_LONG(&const_MARKER_PLUS_value, 5);
	zend_string *const_MARKER_PLUS_name = zend_string_init_interned("MARKER_PLUS", sizeof("MARKER_PLUS") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_MARKER_PLUS_name, &const_MARKER_PLUS_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_MARKER_PLUS_name);

	zval const_LEGEND_NONE_value;
	ZVAL_LONG(&const_LEGEND_NONE_value, 0);
	zend_string *const_LEGEND_NONE_name = zend_string_init_interned("LEGEND_NONE", sizeof("LEGEND_NONE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LEGEND_NONE_name, &const_LEGEND_NONE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LEGEND_NONE_name);

	zval const_LEGEND_TOP_RIGHT_value;
	ZVAL_LONG(&const_LEGEND_TOP_RIGHT_value, 1);
	zend_string *const_LEGEND_TOP_RIGHT_name = zend_string_init_interned("LEGEND_TOP_RIGHT", sizeof("LEGEND_TOP_RIGHT") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LEGEND_TOP_RIGHT_name, &const_LEGEND_TOP_RIGHT_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LEGEND_TOP_RIGHT_name);

	zval const_LEGEND_TOP_LEFT_value;
	ZVAL_LONG(&const_LEGEND_TOP_LEFT_value, 2);
	zend_string *const_LEGEND_TOP_LEFT_name = zend_string_init_interned("LEGEND_TOP_LEFT", sizeof("LEGEND_TOP_LEFT") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LEGEND_TOP_LEFT_name, &const_LEGEND_TOP_LEFT_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LEGEND_TOP_LEFT_name);

	zval const_LEGEND_BOTTOM_RIGHT_value;
	ZVAL_LONG(&const_LEGEND_BOTTOM_RIGHT_value, 3);
	zend_string *const_LEGEND_BOTTOM_RIGHT_name = zend_string_init_interned("LEGEND_BOTTOM_RIGHT", sizeof("LEGEND_BOTTOM_RIGHT") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LEGEND_BOTTOM_RIGHT_name, &const_LEGEND_BOTTOM_RIGHT_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LEGEND_BOTTOM_RIGHT_name);

	zval const_LEGEND_BOTTOM_LEFT_value;
	ZVAL_LONG(&const_LEGEND_BOTTOM_LEFT_value, 4);
	zend_string *const_LEGEND_BOTTOM_LEFT_name = zend_string_init_interned("LEGEND_BOTTOM_LEFT", sizeof("LEGEND_BOTTOM_LEFT") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LEGEND_BOTTOM_LEFT_name, &const_LEGEND_BOTTOM_LEFT_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LEGEND_BOTTOM_LEFT_name);

	zval const_SCALE_LINEAR_value;
	ZVAL_LONG(&const_SCALE_LINEAR_value, 0);
	zend_string *const_SCALE_LINEAR_name = zend_string_init_interned("SCALE_LINEAR", sizeof("SCALE_LINEAR") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_SCALE_LINEAR_name, &const_SCALE_LINEAR_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_SCALE_LINEAR_name);

	zval const_SCALE_LOG_value;
	ZVAL_LONG(&const_SCALE_LOG_value, 1);
	zend_string *const_SCALE_LOG_name = zend_string_init_interned("SCALE_LOG", sizeof("SCALE_LOG") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_SCALE_LOG_name, &const_SCALE_LOG_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_SCALE_LOG_name);

	zval const_LABEL_NONE_value;
	ZVAL_LONG(&const_LABEL_NONE_value, 0);
	zend_string *const_LABEL_NONE_name = zend_string_init_interned("LABEL_NONE", sizeof("LABEL_NONE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LABEL_NONE_name, &const_LABEL_NONE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LABEL_NONE_name);

	zval const_LABEL_INSIDE_value;
	ZVAL_LONG(&const_LABEL_INSIDE_value, 1);
	zend_string *const_LABEL_INSIDE_name = zend_string_init_interned("LABEL_INSIDE", sizeof("LABEL_INSIDE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LABEL_INSIDE_name, &const_LABEL_INSIDE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LABEL_INSIDE_name);

	zval const_LABEL_OUTSIDE_value;
	ZVAL_LONG(&const_LABEL_OUTSIDE_value, 2);
	zend_string *const_LABEL_OUTSIDE_name = zend_string_init_interned("LABEL_OUTSIDE", sizeof("LABEL_OUTSIDE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LABEL_OUTSIDE_name, &const_LABEL_OUTSIDE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LABEL_OUTSIDE_name);

	zval const_STYLE_CANDLE_value;
	ZVAL_LONG(&const_STYLE_CANDLE_value, 0);
	zend_string *const_STYLE_CANDLE_name = zend_string_init_interned("STYLE_CANDLE", sizeof("STYLE_CANDLE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STYLE_CANDLE_name, &const_STYLE_CANDLE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STYLE_CANDLE_name);

	zval const_STYLE_BAR_value;
	ZVAL_LONG(&const_STYLE_BAR_value, 1);
	zend_string *const_STYLE_BAR_name = zend_string_init_interned("STYLE_BAR", sizeof("STYLE_BAR") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STYLE_BAR_name, &const_STYLE_BAR_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STYLE_BAR_name);

	zval const_STYLE_DIAMOND_value;
	ZVAL_LONG(&const_STYLE_DIAMOND_value, 2);
	zend_string *const_STYLE_DIAMOND_name = zend_string_init_interned("STYLE_DIAMOND", sizeof("STYLE_DIAMOND") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STYLE_DIAMOND_name, &const_STYLE_DIAMOND_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STYLE_DIAMOND_name);

	zval const_STYLE_I_CAP_value;
	ZVAL_LONG(&const_STYLE_I_CAP_value, 3);
	zend_string *const_STYLE_I_CAP_name = zend_string_init_interned("STYLE_I_CAP", sizeof("STYLE_I_CAP") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STYLE_I_CAP_name, &const_STYLE_I_CAP_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STYLE_I_CAP_name);

	zval const_STYLE_HOLLOW_value;
	ZVAL_LONG(&const_STYLE_HOLLOW_value, 4);
	zend_string *const_STYLE_HOLLOW_name = zend_string_init_interned("STYLE_HOLLOW", sizeof("STYLE_HOLLOW") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STYLE_HOLLOW_name, &const_STYLE_HOLLOW_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STYLE_HOLLOW_name);

	zval const_STYLE_VOLUME_value;
	ZVAL_LONG(&const_STYLE_VOLUME_value, 5);
	zend_string *const_STYLE_VOLUME_name = zend_string_init_interned("STYLE_VOLUME", sizeof("STYLE_VOLUME") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STYLE_VOLUME_name, &const_STYLE_VOLUME_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STYLE_VOLUME_name);

	zval const_STYLE_VECTOR_value;
	ZVAL_LONG(&const_STYLE_VECTOR_value, 6);
	zend_string *const_STYLE_VECTOR_name = zend_string_init_interned("STYLE_VECTOR", sizeof("STYLE_VECTOR") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STYLE_VECTOR_name, &const_STYLE_VECTOR_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STYLE_VECTOR_name);

	zval const_BORDER_NONE_value;
	ZVAL_LONG(&const_BORDER_NONE_value, 0);
	zend_string *const_BORDER_NONE_name = zend_string_init_interned("BORDER_NONE", sizeof("BORDER_NONE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_BORDER_NONE_name, &const_BORDER_NONE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_BORDER_NONE_name);

	zval const_BORDER_LEFT_value;
	ZVAL_LONG(&const_BORDER_LEFT_value, 1);
	zend_string *const_BORDER_LEFT_name = zend_string_init_interned("BORDER_LEFT", sizeof("BORDER_LEFT") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_BORDER_LEFT_name, &const_BORDER_LEFT_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_BORDER_LEFT_name);

	zval const_BORDER_RIGHT_value;
	ZVAL_LONG(&const_BORDER_RIGHT_value, 2);
	zend_string *const_BORDER_RIGHT_name = zend_string_init_interned("BORDER_RIGHT", sizeof("BORDER_RIGHT") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_BORDER_RIGHT_name, &const_BORDER_RIGHT_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_BORDER_RIGHT_name);

	zval const_BORDER_TOP_value;
	ZVAL_LONG(&const_BORDER_TOP_value, 4);
	zend_string *const_BORDER_TOP_name = zend_string_init_interned("BORDER_TOP", sizeof("BORDER_TOP") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_BORDER_TOP_name, &const_BORDER_TOP_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_BORDER_TOP_name);

	zval const_BORDER_BOTTOM_value;
	ZVAL_LONG(&const_BORDER_BOTTOM_value, 8);
	zend_string *const_BORDER_BOTTOM_name = zend_string_init_interned("BORDER_BOTTOM", sizeof("BORDER_BOTTOM") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_BORDER_BOTTOM_name, &const_BORDER_BOTTOM_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_BORDER_BOTTOM_name);

	zval const_BORDER_ALL_value;
	ZVAL_LONG(&const_BORDER_ALL_value, 15);
	zend_string *const_BORDER_ALL_name = zend_string_init_interned("BORDER_ALL", sizeof("BORDER_ALL") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_BORDER_ALL_name, &const_BORDER_ALL_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_BORDER_ALL_name);

	zval const_INTERP_LINEAR_value;
	ZVAL_LONG(&const_INTERP_LINEAR_value, 0);
	zend_string *const_INTERP_LINEAR_name = zend_string_init_interned("INTERP_LINEAR", sizeof("INTERP_LINEAR") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_INTERP_LINEAR_name, &const_INTERP_LINEAR_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_INTERP_LINEAR_name);

	zval const_INTERP_SMOOTH_value;
	ZVAL_LONG(&const_INTERP_SMOOTH_value, 1);
	zend_string *const_INTERP_SMOOTH_name = zend_string_init_interned("INTERP_SMOOTH", sizeof("INTERP_SMOOTH") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_INTERP_SMOOTH_name, &const_INTERP_SMOOTH_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_INTERP_SMOOTH_name);

	zval const_INTERP_STEP_AFTER_value;
	ZVAL_LONG(&const_INTERP_STEP_AFTER_value, 2);
	zend_string *const_INTERP_STEP_AFTER_name = zend_string_init_interned("INTERP_STEP_AFTER", sizeof("INTERP_STEP_AFTER") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_INTERP_STEP_AFTER_name, &const_INTERP_STEP_AFTER_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_INTERP_STEP_AFTER_name);

	zval const_INTERP_STEP_BEFORE_value;
	ZVAL_LONG(&const_INTERP_STEP_BEFORE_value, 3);
	zend_string *const_INTERP_STEP_BEFORE_name = zend_string_init_interned("INTERP_STEP_BEFORE", sizeof("INTERP_STEP_BEFORE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_INTERP_STEP_BEFORE_name, &const_INTERP_STEP_BEFORE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_INTERP_STEP_BEFORE_name);

	zval const_TICK_NONE_value;
	ZVAL_LONG(&const_TICK_NONE_value, 0);
	zend_string *const_TICK_NONE_name = zend_string_init_interned("TICK_NONE", sizeof("TICK_NONE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_TICK_NONE_name, &const_TICK_NONE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_TICK_NONE_name);

	zval const_TICK_LABELS_value;
	ZVAL_LONG(&const_TICK_LABELS_value, 1);
	zend_string *const_TICK_LABELS_name = zend_string_init_interned("TICK_LABELS", sizeof("TICK_LABELS") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_TICK_LABELS_name, &const_TICK_LABELS_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_TICK_LABELS_name);

	zval const_TICK_POINTS_value;
	ZVAL_LONG(&const_TICK_POINTS_value, 2);
	zend_string *const_TICK_POINTS_name = zend_string_init_interned("TICK_POINTS", sizeof("TICK_POINTS") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_TICK_POINTS_name, &const_TICK_POINTS_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_TICK_POINTS_name);

	zval const_TICK_BOTH_value;
	ZVAL_LONG(&const_TICK_BOTH_value, 3);
	zend_string *const_TICK_BOTH_name = zend_string_init_interned("TICK_BOTH", sizeof("TICK_BOTH") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_TICK_BOTH_name, &const_TICK_BOTH_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_TICK_BOTH_name);

	zval const_STACK_SUM_value;
	ZVAL_LONG(&const_STACK_SUM_value, 0);
	zend_string *const_STACK_SUM_name = zend_string_init_interned("STACK_SUM", sizeof("STACK_SUM") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STACK_SUM_name, &const_STACK_SUM_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STACK_SUM_name);

	zval const_STACK_BESIDE_value;
	ZVAL_LONG(&const_STACK_BESIDE_value, 1);
	zend_string *const_STACK_BESIDE_name = zend_string_init_interned("STACK_BESIDE", sizeof("STACK_BESIDE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STACK_BESIDE_name, &const_STACK_BESIDE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STACK_BESIDE_name);

	zval const_STACK_LAYER_value;
	ZVAL_LONG(&const_STACK_LAYER_value, 2);
	zend_string *const_STACK_LAYER_name = zend_string_init_interned("STACK_LAYER", sizeof("STACK_LAYER") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STACK_LAYER_name, &const_STACK_LAYER_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STACK_LAYER_name);

	zval const_LABEL_LEFT_value;
	ZVAL_LONG(&const_LABEL_LEFT_value, 3);
	zend_string *const_LABEL_LEFT_name = zend_string_init_interned("LABEL_LEFT", sizeof("LABEL_LEFT") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LABEL_LEFT_name, &const_LABEL_LEFT_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LABEL_LEFT_name);

	zval const_LABEL_RIGHT_value;
	ZVAL_LONG(&const_LABEL_RIGHT_value, 4);
	zend_string *const_LABEL_RIGHT_name = zend_string_init_interned("LABEL_RIGHT", sizeof("LABEL_RIGHT") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LABEL_RIGHT_name, &const_LABEL_RIGHT_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LABEL_RIGHT_name);

	zval const_LINE_SOLID_value;
	ZVAL_LONG(&const_LINE_SOLID_value, 0);
	zend_string *const_LINE_SOLID_name = zend_string_init_interned("LINE_SOLID", sizeof("LINE_SOLID") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LINE_SOLID_name, &const_LINE_SOLID_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LINE_SOLID_name);

	zval const_LINE_DASHED_value;
	ZVAL_LONG(&const_LINE_DASHED_value, 1);
	zend_string *const_LINE_DASHED_name = zend_string_init_interned("LINE_DASHED", sizeof("LINE_DASHED") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LINE_DASHED_name, &const_LINE_DASHED_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LINE_DASHED_name);

	zval const_LINE_DOTTED_value;
	ZVAL_LONG(&const_LINE_DOTTED_value, 2);
	zend_string *const_LINE_DOTTED_name = zend_string_init_interned("LINE_DOTTED", sizeof("LINE_DOTTED") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_LINE_DOTTED_name, &const_LINE_DOTTED_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_LINE_DOTTED_name);

	zval const_GRADIENT_VERTICAL_value;
	ZVAL_LONG(&const_GRADIENT_VERTICAL_value, 0);
	zend_string *const_GRADIENT_VERTICAL_name = zend_string_init_interned("GRADIENT_VERTICAL", sizeof("GRADIENT_VERTICAL") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_GRADIENT_VERTICAL_name, &const_GRADIENT_VERTICAL_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_GRADIENT_VERTICAL_name);

	zval const_GRADIENT_HORIZONTAL_value;
	ZVAL_LONG(&const_GRADIENT_HORIZONTAL_value, 1);
	zend_string *const_GRADIENT_HORIZONTAL_name = zend_string_init_interned("GRADIENT_HORIZONTAL", sizeof("GRADIENT_HORIZONTAL") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_GRADIENT_HORIZONTAL_name, &const_GRADIENT_HORIZONTAL_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_GRADIENT_HORIZONTAL_name);

	zval const_DATE_DAY_value;
	ZVAL_LONG(&const_DATE_DAY_value, 0);
	zend_string *const_DATE_DAY_name = zend_string_init_interned("DATE_DAY", sizeof("DATE_DAY") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_DATE_DAY_name, &const_DATE_DAY_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_DATE_DAY_name);

	zval const_DATE_WEEK_value;
	ZVAL_LONG(&const_DATE_WEEK_value, 1);
	zend_string *const_DATE_WEEK_name = zend_string_init_interned("DATE_WEEK", sizeof("DATE_WEEK") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_DATE_WEEK_name, &const_DATE_WEEK_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_DATE_WEEK_name);

	zval const_DATE_MONTH_value;
	ZVAL_LONG(&const_DATE_MONTH_value, 2);
	zend_string *const_DATE_MONTH_name = zend_string_init_interned("DATE_MONTH", sizeof("DATE_MONTH") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_DATE_MONTH_name, &const_DATE_MONTH_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_DATE_MONTH_name);

	zval const_DATE_QUARTER_value;
	ZVAL_LONG(&const_DATE_QUARTER_value, 3);
	zend_string *const_DATE_QUARTER_name = zend_string_init_interned("DATE_QUARTER", sizeof("DATE_QUARTER") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_DATE_QUARTER_name, &const_DATE_QUARTER_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_DATE_QUARTER_name);

	zval const_DATE_YEAR_value;
	ZVAL_LONG(&const_DATE_YEAR_value, 4);
	zend_string *const_DATE_YEAR_name = zend_string_init_interned("DATE_YEAR", sizeof("DATE_YEAR") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_DATE_YEAR_name, &const_DATE_YEAR_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_DATE_YEAR_name);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_LineChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "LineChart", class_FastChart_LineChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_AreaChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "AreaChart", class_FastChart_AreaChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_BarChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "BarChart", class_FastChart_BarChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	zval const_BAR_VERTICAL_value;
	ZVAL_LONG(&const_BAR_VERTICAL_value, 0);
	zend_string *const_BAR_VERTICAL_name = zend_string_init_interned("BAR_VERTICAL", sizeof("BAR_VERTICAL") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_BAR_VERTICAL_name, &const_BAR_VERTICAL_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_BAR_VERTICAL_name);

	zval const_BAR_HORIZONTAL_value;
	ZVAL_LONG(&const_BAR_HORIZONTAL_value, 1);
	zend_string *const_BAR_HORIZONTAL_name = zend_string_init_interned("BAR_HORIZONTAL", sizeof("BAR_HORIZONTAL") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_BAR_HORIZONTAL_name, &const_BAR_HORIZONTAL_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_BAR_HORIZONTAL_name);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_PieChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "PieChart", class_FastChart_PieChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_ScatterChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "ScatterChart", class_FastChart_ScatterChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_StockChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "StockChart", class_FastChart_StockChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	zval const_MA_SMA_value;
	ZVAL_LONG(&const_MA_SMA_value, 0);
	zend_string *const_MA_SMA_name = zend_string_init_interned("MA_SMA", sizeof("MA_SMA") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_MA_SMA_name, &const_MA_SMA_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_MA_SMA_name);

	zval const_MA_EMA_value;
	ZVAL_LONG(&const_MA_EMA_value, 1);
	zend_string *const_MA_EMA_name = zend_string_init_interned("MA_EMA", sizeof("MA_EMA") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_MA_EMA_name, &const_MA_EMA_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_MA_EMA_name);

	zval const_MA_WMA_value;
	ZVAL_LONG(&const_MA_WMA_value, 2);
	zend_string *const_MA_WMA_name = zend_string_init_interned("MA_WMA", sizeof("MA_WMA") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_MA_WMA_name, &const_MA_WMA_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_MA_WMA_name);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_RadarChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "RadarChart", class_FastChart_RadarChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_BubbleChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "BubbleChart", class_FastChart_BubbleChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_SurfaceChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "SurfaceChart", class_FastChart_SurfaceChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_GaugeChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "GaugeChart", class_FastChart_GaugeChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_GanttChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "GanttChart", class_FastChart_GanttChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_BoxPlot(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "BoxPlot", class_FastChart_BoxPlot_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_PolarChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "PolarChart", class_FastChart_PolarChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	zval const_STYLE_LINE_value;
	ZVAL_LONG(&const_STYLE_LINE_value, 0);
	zend_string *const_STYLE_LINE_name = zend_string_init_interned("STYLE_LINE", sizeof("STYLE_LINE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STYLE_LINE_name, &const_STYLE_LINE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STYLE_LINE_name);

	zval const_STYLE_ROSE_value;
	ZVAL_LONG(&const_STYLE_ROSE_value, 1);
	zend_string *const_STYLE_ROSE_name = zend_string_init_interned("STYLE_ROSE", sizeof("STYLE_ROSE") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_STYLE_ROSE_name, &const_STYLE_ROSE_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_STYLE_ROSE_name);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_ContourChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "ContourChart", class_FastChart_ContourChart_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_Treemap(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "Treemap", class_FastChart_Treemap_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_Funnel(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "Funnel", class_FastChart_Funnel_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_Waterfall(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "Waterfall", class_FastChart_Waterfall_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_Heatmap(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "Heatmap", class_FastChart_Heatmap_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_LinearMeter(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "LinearMeter", class_FastChart_LinearMeter_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Chart, ZEND_ACC_FINAL);

	zval const_METER_HORIZONTAL_value;
	ZVAL_LONG(&const_METER_HORIZONTAL_value, 0);
	zend_string *const_METER_HORIZONTAL_name = zend_string_init_interned("METER_HORIZONTAL", sizeof("METER_HORIZONTAL") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_METER_HORIZONTAL_name, &const_METER_HORIZONTAL_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_METER_HORIZONTAL_name);

	zval const_METER_VERTICAL_value;
	ZVAL_LONG(&const_METER_VERTICAL_value, 1);
	zend_string *const_METER_VERTICAL_name = zend_string_init_interned("METER_VERTICAL", sizeof("METER_VERTICAL") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_METER_VERTICAL_name, &const_METER_VERTICAL_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_METER_VERTICAL_name);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_Symbol(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "Symbol", class_FastChart_Symbol_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, ZEND_ACC_ABSTRACT);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_Barcode(zend_class_entry *class_entry_FastChart_Symbol)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "Barcode", NULL);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Symbol, ZEND_ACC_ABSTRACT);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_Code128(zend_class_entry *class_entry_FastChart_Barcode)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "Code128", class_FastChart_Code128_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Barcode, ZEND_ACC_FINAL);

	return class_entry;
}

static zend_class_entry *register_class_FastChart_QrCode(zend_class_entry *class_entry_FastChart_Symbol)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "QrCode", class_FastChart_QrCode_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_FastChart_Symbol, ZEND_ACC_FINAL);

	zval const_ECC_L_value;
	ZVAL_LONG(&const_ECC_L_value, 0);
	zend_string *const_ECC_L_name = zend_string_init_interned("ECC_L", sizeof("ECC_L") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_ECC_L_name, &const_ECC_L_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_ECC_L_name);

	zval const_ECC_M_value;
	ZVAL_LONG(&const_ECC_M_value, 1);
	zend_string *const_ECC_M_name = zend_string_init_interned("ECC_M", sizeof("ECC_M") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_ECC_M_name, &const_ECC_M_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_ECC_M_name);

	zval const_ECC_Q_value;
	ZVAL_LONG(&const_ECC_Q_value, 2);
	zend_string *const_ECC_Q_name = zend_string_init_interned("ECC_Q", sizeof("ECC_Q") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_ECC_Q_name, &const_ECC_Q_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_ECC_Q_name);

	zval const_ECC_H_value;
	ZVAL_LONG(&const_ECC_H_value, 3);
	zend_string *const_ECC_H_name = zend_string_init_interned("ECC_H", sizeof("ECC_H") - 1, 1);
	zend_declare_typed_class_constant(class_entry, const_ECC_H_name, &const_ECC_H_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(const_ECC_H_name);

	return class_entry;
}
