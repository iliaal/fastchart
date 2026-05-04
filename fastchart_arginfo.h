/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 52ba6d350dd2dc6bf8d3a25e6c97866a273d4f76 */

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

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_FastChart_Chart_draw, 0, 1, GdImage, 0)
	ZEND_ARG_OBJ_INFO(0, canvas, GdImage, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_LineChart_setSeries, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, series, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_LineChart_draw arginfo_class_FastChart_Chart_draw

#define arginfo_class_FastChart_BarChart_setSeries arginfo_class_FastChart_LineChart_setSeries

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_BarChart_setStacked, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, stacked, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_BarChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_PieChart_setSlices, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, slices, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_PieChart_setDonutHoleRatio, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, ratio, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_PieChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_ScatterChart_setPoints, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, points, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_ScatterChart_draw arginfo_class_FastChart_Chart_draw

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_setOhlcv, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, ohlcv, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_setMovingAverages, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, periods, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_FastChart_StockChart_setVolumePane, 0, 1, IS_STATIC, 0)
	ZEND_ARG_TYPE_INFO(0, enabled, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FastChart_StockChart_draw arginfo_class_FastChart_Chart_draw

ZEND_METHOD(FastChart_Chart, version);
ZEND_METHOD(FastChart_Chart, setSize);
ZEND_METHOD(FastChart_Chart, setTitle);
ZEND_METHOD(FastChart_Chart, setTheme);
ZEND_METHOD(FastChart_LineChart, setSeries);
ZEND_METHOD(FastChart_LineChart, draw);
ZEND_METHOD(FastChart_BarChart, setSeries);
ZEND_METHOD(FastChart_BarChart, setStacked);
ZEND_METHOD(FastChart_BarChart, draw);
ZEND_METHOD(FastChart_PieChart, setSlices);
ZEND_METHOD(FastChart_PieChart, setDonutHoleRatio);
ZEND_METHOD(FastChart_PieChart, draw);
ZEND_METHOD(FastChart_ScatterChart, setPoints);
ZEND_METHOD(FastChart_ScatterChart, draw);
ZEND_METHOD(FastChart_StockChart, setOhlcv);
ZEND_METHOD(FastChart_StockChart, setMovingAverages);
ZEND_METHOD(FastChart_StockChart, setVolumePane);
ZEND_METHOD(FastChart_StockChart, draw);

static const zend_function_entry class_FastChart_Chart_methods[] = {
	ZEND_ME(FastChart_Chart, version, arginfo_class_FastChart_Chart_version, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(FastChart_Chart, setSize, arginfo_class_FastChart_Chart_setSize, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setTitle, arginfo_class_FastChart_Chart_setTitle, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_Chart, setTheme, arginfo_class_FastChart_Chart_setTheme, ZEND_ACC_PUBLIC)
	ZEND_RAW_FENTRY("draw", NULL, arginfo_class_FastChart_Chart_draw, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT, NULL, NULL)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_LineChart_methods[] = {
	ZEND_ME(FastChart_LineChart, setSeries, arginfo_class_FastChart_LineChart_setSeries, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_LineChart, draw, arginfo_class_FastChart_LineChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_BarChart_methods[] = {
	ZEND_ME(FastChart_BarChart, setSeries, arginfo_class_FastChart_BarChart_setSeries, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_BarChart, setStacked, arginfo_class_FastChart_BarChart_setStacked, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_BarChart, draw, arginfo_class_FastChart_BarChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_PieChart_methods[] = {
	ZEND_ME(FastChart_PieChart, setSlices, arginfo_class_FastChart_PieChart_setSlices, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PieChart, setDonutHoleRatio, arginfo_class_FastChart_PieChart_setDonutHoleRatio, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_PieChart, draw, arginfo_class_FastChart_PieChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_ScatterChart_methods[] = {
	ZEND_ME(FastChart_ScatterChart, setPoints, arginfo_class_FastChart_ScatterChart_setPoints, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_ScatterChart, draw, arginfo_class_FastChart_ScatterChart_draw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_FastChart_StockChart_methods[] = {
	ZEND_ME(FastChart_StockChart, setOhlcv, arginfo_class_FastChart_StockChart_setOhlcv, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, setMovingAverages, arginfo_class_FastChart_StockChart_setMovingAverages, ZEND_ACC_PUBLIC)
	ZEND_ME(FastChart_StockChart, setVolumePane, arginfo_class_FastChart_StockChart_setVolumePane, ZEND_ACC_PUBLIC)
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

	return class_entry;
}

static zend_class_entry *register_class_FastChart_LineChart(zend_class_entry *class_entry_FastChart_Chart)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "FastChart", "LineChart", class_FastChart_LineChart_methods);
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
