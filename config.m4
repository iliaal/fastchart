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

  dnl libgd is required: ext/gd exposes only php_gd_libgdimageptr_from_zval_p()
  dnl so we link libgd directly to call the drawing primitives (gdImageLine,
  dnl gdImageFilledRectangle, gdImageFilledArc, gdImageStringFT, etc.) on the
  dnl gdImagePtr we pull out of caller-supplied \GdImage zvals. The dynamic
  dnl linker dedupes libgd.so.3 so ext/gd and fastchart share one copy in
  dnl the address space.
  dnl
  dnl gdImageStringFT (TTF text rendering) requires libgd to be built with
  dnl FreeType support. The probe checks for the symbol's presence; libgd
  dnl on Debian/Ubuntu/RHEL stable all ship --with-freetype by default.
  PHP_CHECK_LIBRARY(gd, gdImageCreateTrueColor,
  [
    PHP_ADD_LIBRARY(gd, 1, FASTCHART_SHARED_LIBADD)
    AC_DEFINE(HAVE_LIBGD, 1, [Have libgd])
  ],[
    AC_MSG_ERROR([libgd not found. Install libgd-dev (Debian/Ubuntu) or gd-devel (RHEL).])
  ])

  PHP_CHECK_LIBRARY(gd, gdImageStringFT,
  [
    AC_DEFINE(HAVE_GD_FREETYPE, 1, [libgd has FreeType / gdImageStringFT])
  ],[
    AC_MSG_ERROR([libgd was built without FreeType support; gdImageStringFT is unavailable. Rebuild libgd with --with-freetype.])
  ])

  dnl AVIF support is optional. Older libgd builds and stripped distros
  dnl ship without it; the renderer raises a runtime exception when
  dnl AVIF is requested on a build that doesn't expose the symbol.
  PHP_CHECK_LIBRARY(gd, gdImageAvifPtrEx,
  [
    AC_DEFINE(HAVE_GD_AVIF, 1, [libgd has AVIF / gdImageAvifPtrEx])
  ],[])

  PHP_SUBST(FASTCHART_SHARED_LIBADD)

  dnl ext/gd must load before fastchart so php_gd_libgdimageptr_from_zval_p
  dnl is resolvable at fastchart's MINIT and \GdImage class entries are
  dnl registered before any FastChart method dereferences a passed canvas.
  PHP_ADD_EXTENSION_DEP(fastchart, gd)

  WRAPPER_SOURCES="fastchart.c \
    fastchart_palette.c \
    fastchart_text.c \
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
    fastchart_effects.c \
    fastchart_aa.c"

  dnl -Wall -Wextra are on by default so wrapper regressions get caught
  dnl in every local build; --enable-fastchart-dev upgrades warnings to
  dnl -Werror plus extra strictness.
  FASTCHART_CFLAGS="-Wall -Wextra -Wno-unused-parameter"

  if test "$PHP_FASTCHART_DEV" = "yes"; then
    FASTCHART_CFLAGS="$FASTCHART_CFLAGS -Werror -Wstrict-prototypes"
  fi

  dnl Vendored NanoSVG rasterizer header lives under vendor/nanosvg/.
  dnl It's a single-translation-unit body in fastchart_aa.c (defines
  dnl NANOSVGRAST_IMPLEMENTATION before #include) so no separate compile
  dnl unit; we only need the include path. Disable -Wstrict-prototypes
  dnl just for fastchart_aa.c since the upstream rasterizer header
  dnl uses old-style prototypes that trip --enable-fastchart-dev.
  PHP_ADD_INCLUDE([$ext_srcdir])
  PHP_ADD_BUILD_DIR([$ext_builddir/vendor/nanosvg], 1)

  PHP_NEW_EXTENSION(fastchart,
    $WRAPPER_SOURCES,
    $ext_shared,,
    $FASTCHART_CFLAGS)
fi
