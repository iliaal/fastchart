/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 4e6cc25f2f9de3d56d4e061aebed577536666048 */

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

#define arginfo_class_FastChart_Chart_setXAxisFont arginfo_class_FastChart_Chart_setTitleFont

#define arginfo_class_FastChart_Chart_setYAxisFont arginfo_class_FastChart_Chart_setTitleFont

#define arginfo_class_FastChart_Chart_setXAxisTitleFont arginfo_class_FastChart_Chart_setTitleFont

#define arginfo_class_FastChart_Chart_setYAxisTitleFont arginfo_class_FastChart_Chart_setTitleFont

#define arginfo_class_FastChart_Chart_setAnnotationFont arginfo_class_FastChart_Chart_setTitleFont

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_Chart_addTextAnnotation, 0, 3, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, text, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, x, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, y, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, color, IS_LONG, 1, "null")
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

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_LineChart_setSeries, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, series, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_LineChart_setMarkerStyle, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, style, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_LineChart_setMarkerSize, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, size, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_LineChart_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_AreaChart_setSeries arginfo_class_FastChart_LineChart_setSeries

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_AreaChart_setStacked, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, stacked, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_AreaChart_setFillOpacity, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, alpha, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_AreaChart_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_BarChart_setSeries arginfo_class_FastChart_LineChart_setSeries

#define arginfo_class_FastChart_BarChart_setStacked arginfo_class_FastChart_AreaChart_setStacked

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

#define arginfo_class_FastChart_ScatterChart_setMarkerStyle arginfo_class_FastChart_LineChart_setMarkerStyle

#define arginfo_class_FastChart_ScatterChart_setMarkerSize arginfo_class_FastChart_LineChart_setMarkerSize

#define arginfo_class_FastChart_ScatterChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_setOhlcv, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, ohlcv, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_setMovingAverages, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, periods, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_StockChart_setVolumePane arginfo_class_FastChart_Chart_setSecondaryYAxis

#define arginfo_class_FastChart_StockChart_setVolumeColors arginfo_class_FastChart_Chart_setSeriesColors

#define arginfo_class_FastChart_StockChart_setCandleStyle arginfo_class_FastChart_LineChart_setMarkerStyle

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_addIndicatorPane, 0, 2, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, values, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, opts, IS_ARRAY, 1, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_StockChart_draw arginfo_class_FastChart_Chart_draw

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
ZEND_METHOD(FastChart_Chart, setXAxisFont);
ZEND_METHOD(FastChart_Chart, setYAxisFont);
ZEND_METHOD(FastChart_Chart, setXAxisTitleFont);
ZEND_METHOD(FastChart_Chart, setYAxisTitleFont);
ZEND_METHOD(FastChart_Chart, setAnnotationFont);
ZEND_METHOD(FastChart_Chart, addTextAnnotation);
ZEND_METHOD(FastChart_Chart, renderPng);
ZEND_METHOD(FastChart_Chart, renderJpeg);
ZEND_METHOD(FastChart_Chart, renderWebp);
ZEND_METHOD(FastChart_Chart, renderGif);
ZEND_METHOD(FastChart_Chart, renderAvif);
ZEND_METHOD(FastChart_Chart, renderToFile);
ZEND_METHOD(FastChart_LineChart, setSeries);
ZEND_METHOD(FastChart_LineChart, setMarkerStyle);
ZEND_METHOD(FastChart_LineChart, setMarkerSize);
ZEND_METHOD(FastChart_LineChart, draw);
ZEND_METHOD(FastChart_AreaChart, setSeries);
ZEND_METHOD(FastChart_AreaChart, setStacked);
ZEND_METHOD(FastChart_AreaChart, setFillOpacity);
ZEND_METHOD(FastChart_AreaChart, draw);
ZEND_METHOD(FastChart_BarChart, setSeries);
ZEND_METHOD(FastChart_BarChart, setStacked);
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
ZEND_METHOD(FastChart_ScatterChart, draw);
ZEND_METHOD(FastChart_StockChart, setOhlcv);
ZEND_METHOD(FastChart_StockChart, setMovingAverages);
ZEND_METHOD(FastChart_StockChart, setVolumePane);
ZEND_METHOD(FastChart_StockChart, setVolumeColors);
ZEND_METHOD(FastChart_StockChart, setCandleStyle);
ZEND_METHOD(FastChart_StockChart, addIndicatorPane);
ZEND_METHOD(FastChart_StockChart, draw);

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
	ZEND_ME(FastChart_Chart, setXAxisFont, arginfo_class_FastChart_Chart_setXAxisFont, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setYAxisFont, arginfo_class_FastChart_Chart_setYAxisFont, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setXAxisTitleFont, arginfo_class_FastChart_Chart_setXAxisTitleFont, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setYAxisTitleFont, arginfo_class_FastChart_Chart_setYAxisTitleFont, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setAnnotationFont, arginfo_class_FastChart_Chart_setAnnotationFont, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, addTextAnnotation, arginfo_class_FastChart_Chart_addTextAnnotation, ZEND_ACC_PUBLIC)
	ZEND_RAW_FENTRY("draw", NULL, arginfo_class_FastChart_Chart_draw, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT, NULL, NULL)
	ZEND_ME(FastChart_Chart, renderPng, arginfo_class_FastChart_Chart_renderPng, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, renderJpeg, arginfo_class_FastChart_Chart_renderJpeg, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, renderWebp, arginfo_class_FastChart_Chart_renderWebp, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, renderGif, arginfo_class_FastChart_Chart_renderGif, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, renderAvif, arginfo_class_FastChart_Chart_renderAvif, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, renderToFile, arginfo_class_FastChart_Chart_renderToFile, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_LineChart_methods[] = {
	ZEND_ME(FastChart_LineChart, setSeries, arginfo_class_FastChart_LineChart_setSeries, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LineChart, setMarkerStyle, arginfo_class_FastChart_LineChart_setMarkerStyle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LineChart, setMarkerSize, arginfo_class_FastChart_LineChart_setMarkerSize, ZEND_ACC_PUBLIC)
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
	ZEND_ME(FastChart_ScatterChart, draw, arginfo_class_FastChart_ScatterChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_StockChart_methods[] = {
	ZEND_ME(FastChart_StockChart, setOhlcv, arginfo_class_FastChart_StockChart_setOhlcv, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, setMovingAverages, arginfo_class_FastChart_StockChart_setMovingAverages, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, setVolumePane, arginfo_class_FastChart_StockChart_setVolumePane, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, setVolumeColors, arginfo_class_FastChart_StockChart_setVolumeColors, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, setCandleStyle, arginfo_class_FastChart_StockChart_setCandleStyle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, addIndicatorPane, arginfo_class_FastChart_StockChart_addIndicatorPane, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, draw, arginfo_class_FastChart_StockChart_draw, ZEND_ACC_PUBLIC)
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

	return class_entry;
}
