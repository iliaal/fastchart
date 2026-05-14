dnl config.m4 for extension fastchart

PHP_ARG_ENABLE(fastchart, whether to enable fastchart support,
[  --enable-fastchart      Enable fastchart support])

PHP_ARG_ENABLE(fastchart-dev, whether to enable developer build flags,
[  --enable-fastchart-dev  Upgrade wrapper warnings to -Werror plus strict checks], no, no)

if test "$PHP_FASTCHART" != "no"; then

  PHP_VERSION_ID=$($PHP_CONFIG --vernum)
  if test "$PHP_VERSION_ID" -lt "80300"; then
    AC_MSG_ERROR([fastchart requires PHP 8.3.0 or later (found $PHP_VERSION_ID)])
  fi

  dnl ----- pkg-config-resolved deps ---------------------------------------
  dnl FreeType: mandatory. Glyph metrics + family-name resolution for
  dnl SVG output; plus FT_Outline_Decompose for glyph-to-path emission
  dnl in SVG_TEXT_PATHS mode, and text bbox measurement for chart layout.
  dnl
  dnl libpng / libjpeg-turbo / libwebp: optional, probed independently.
  dnl Each missing lib drops its corresponding output format — the
  dnl encoder compiles to a stub that returns -2 and the PHP method
  dnl throws a clear "format not compiled in" error at call time.
  AC_PATH_PROG(FC_PKGCFG, pkg-config, no)
  if test "$FC_PKGCFG" = "no"; then
    AC_MSG_ERROR([pkg-config not found. Install pkg-config (Debian/Ubuntu) or pkgconf (RHEL).])
  fi

  if ! $FC_PKGCFG --exists freetype2; then
    AC_MSG_ERROR([pkg-config freetype2 not found. Install libfreetype-dev / freetype-devel — FreeType is required for text rendering.])
  fi

  FC_PC_CFLAGS=`$FC_PKGCFG --cflags freetype2`
  FC_PC_LIBS=`$FC_PKGCFG --libs   freetype2`
  FC_OPT_DEFS=""

  for fc_pair in libpng:HAVE_LIBPNG libjpeg:HAVE_LIBJPEG libwebp:HAVE_LIBWEBP; do
    fc_lib=`echo $fc_pair | cut -d: -f1`
    fc_def=`echo $fc_pair | cut -d: -f2`
    if $FC_PKGCFG --exists $fc_lib; then
      FC_PC_CFLAGS="$FC_PC_CFLAGS `$FC_PKGCFG --cflags $fc_lib`"
      FC_PC_LIBS="$FC_PC_LIBS `$FC_PKGCFG --libs $fc_lib`"
      FC_OPT_DEFS="$FC_OPT_DEFS -D$fc_def=1"
      AC_MSG_NOTICE([fastchart: $fc_lib found, $fc_def enabled])
    else
      AC_MSG_NOTICE([fastchart: $fc_lib not found — corresponding output format will be unavailable at runtime])
    fi
  done

  PHP_EVAL_INCLINE([$FC_PC_CFLAGS])
  PHP_EVAL_LIBLINE([$FC_PC_LIBS], FASTCHART_SHARED_LIBADD)

  PHP_SUBST(FASTCHART_SHARED_LIBADD)

  WRAPPER_SOURCES="fastchart.c \
    fastchart_palette.c \
    fastchart_text.c \
    fastchart_target.c \
    fastchart_svg.c \
    fastchart_axis.c \
    fastchart_line.c \
    fastchart_area.c \
    fastchart_bar.c \
    fastchart_pie.c \
    fastchart_scatter.c \
    fastchart_stock.c \
    fastchart_radar.c \
    fastchart_bubble.c \
    fastchart_surface.c \
    fastchart_gauge.c \
    fastchart_gantt.c \
    fastchart_boxplot.c \
    fastchart_polar.c \
    fastchart_contour.c \
    fastchart_treemap.c \
    fastchart_funnel.c \
    fastchart_waterfall.c \
    fastchart_heatmap.c \
    fastchart_linear_meter.c \
    fastchart_effects.c \
    fastchart_encoder.c \
    fastchart_rasterize.c \
    fastchart_symbol.c \
    fastchart_code128.c \
    fastchart_qrcode.c \
    vendor/qrcodegen/qrcodegen.c \
    vendor/plutovg/source/plutovg-blend.c \
    vendor/plutovg/source/plutovg-canvas.c \
    vendor/plutovg/source/plutovg-font.c \
    vendor/plutovg/source/plutovg-ft-math.c \
    vendor/plutovg/source/plutovg-ft-raster.c \
    vendor/plutovg/source/plutovg-ft-stroker.c \
    vendor/plutovg/source/plutovg-matrix.c \
    vendor/plutovg/source/plutovg-paint.c \
    vendor/plutovg/source/plutovg-path.c \
    vendor/plutovg/source/plutovg-rasterize.c \
    vendor/plutovg/source/plutovg-surface.c \
    vendor/plutosvg/source/plutosvg.c"

  dnl -Wall -Wextra are on by default so wrapper regressions get caught
  dnl in every local build; --enable-fastchart-dev upgrades warnings to
  dnl -Werror plus extra strictness.
  dnl
  dnl -fvisibility=hidden keeps the vendored plutovg_/plutosvg_/qrcodegen_
  dnl symbols (and every internal fastchart_* helper) out of the dynamic
  dnl symbol table. Only get_module stays exported, marked default by
  dnl ZEND_DLEXPORT. PLUTOVG_BUILD_STATIC + PLUTOSVG_BUILD_STATIC make those
  dnl libraries' headers stop applying visibility=default on their API
  dnl decorations, so -fvisibility=hidden actually buries them.
  dnl Verify with:
  dnl   nm -D --defined-only modules/fastchart.so | grep -v get_module
  dnl Expected: only standard-library symbols (memcpy, etc.) remain.
  dnl Vendor-driven warning suppressions: plutovg ships stb_image* /
  dnl stb_image_write* / stb_truetype headers + ft-stroker code that
  dnl trip a few standard warnings (unused statics, signed/unsigned
  dnl comparisons in tight inner loops, implicit fallthrough in case
  dnl arms). PHP_NEW_EXTENSION applies one CFLAGS to every TU, so we
  dnl silence globally rather than per-file. Fastchart's own code is
  dnl warning-clean under the unsuppressed set.
  FASTCHART_CFLAGS="-Wall -Wextra \
    -Wno-unused-parameter -Wno-unused-function -Wno-sign-compare \
    -Wno-implicit-fallthrough -Wno-unused-but-set-variable \
    -Wno-misleading-indentation -Wno-missing-field-initializers \
    -fvisibility=hidden \
    -DPLUTOVG_BUILD_STATIC -DPLUTOSVG_BUILD_STATIC \
    $FC_OPT_DEFS"

  if test "$PHP_FASTCHART_DEV" = "yes"; then
    FASTCHART_CFLAGS="$FASTCHART_CFLAGS -Werror -Wstrict-prototypes"
  fi

  PHP_ADD_INCLUDE([$ext_srcdir])
  dnl Vendor include paths. Uses $abs_srcdir (autoconf-standard, always
  dnl populated before this macro fires) so VPATH / out-of-tree builds
  dnl resolve correctly. Mirrors ext/fileinfo's libmagic wiring.
  PHP_ADD_INCLUDE([$abs_srcdir/vendor/qrcodegen])
  PHP_ADD_INCLUDE([$abs_srcdir/vendor/plutovg/include])
  PHP_ADD_INCLUDE([$abs_srcdir/vendor/plutovg/source])
  PHP_ADD_INCLUDE([$abs_srcdir/vendor/plutosvg/source])

  dnl PHP_NEW_EXTENSION compiles vendor/*/qrcodegen.c, plutovg-*.c, and
  dnl plutosvg.c into matching .lo files — for VPATH builds the directory
  dnl must exist under $ext_builddir or libtool will fail to write.
  PHP_ADD_BUILD_DIR([$ext_builddir/vendor/qrcodegen])
  PHP_ADD_BUILD_DIR([$ext_builddir/vendor/plutovg/source])
  PHP_ADD_BUILD_DIR([$ext_builddir/vendor/plutosvg/source])

  PHP_NEW_EXTENSION(fastchart,
    $WRAPPER_SOURCES,
    $ext_shared,,
    $FASTCHART_CFLAGS)
fi
