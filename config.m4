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

  dnl FreeType direct access. The SVG output backend reads TTF family
  dnl names via FT_New_Face to emit accurate CSS font-family attributes.
  dnl libgd already loads libfreetype.so at runtime (it's how
  dnl gdImageStringFT works); we additionally need the headers + an
  dnl explicit -lfreetype to call FT_New_Face from fastchart_target.c.
  dnl pkg-config is the portable resolver across Debian/RHEL/macOS.
  AC_PATH_PROG(FC_PKGCFG, pkg-config, no)
  if test "$FC_PKGCFG" = "no" || ! $FC_PKGCFG --exists freetype2; then
    AC_MSG_ERROR([pkg-config freetype2 not found. Install libfreetype-dev (Debian/Ubuntu) or freetype-devel (RHEL).])
  fi
  FREETYPE_CFLAGS=`$FC_PKGCFG --cflags freetype2`
  FREETYPE_LIBS=`$FC_PKGCFG --libs freetype2`
  PHP_EVAL_INCLINE([$FREETYPE_CFLAGS])
  PHP_EVAL_LIBLINE([$FREETYPE_LIBS], FASTCHART_SHARED_LIBADD)

  PHP_SUBST(FASTCHART_SHARED_LIBADD)

  dnl ext/gd must load before fastchart so php_gd_libgdimageptr_from_zval_p
  dnl is resolvable at fastchart's MINIT and \GdImage class entries are
  dnl registered before any FastChart method dereferences a passed canvas.
  PHP_ADD_EXTENSION_DEP(fastchart, gd)

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
    fastchart_symbol.c \
    fastchart_code128.c \
    fastchart_qrcode.c \
    vendor/qrcodegen/qrcodegen.c"

  dnl -Wall -Wextra are on by default so wrapper regressions get caught
  dnl in every local build; --enable-fastchart-dev upgrades warnings to
  dnl -Werror plus extra strictness.
  dnl
  dnl -fvisibility=hidden keeps the vendored qrcodegen_* symbols (and
  dnl every internal fastchart_* helper) out of the dynamic symbol
  dnl table. Only get_module stays exported, marked default by
  dnl ZEND_DLEXPORT. Without this, another extension loading a
  dnl different qrcodegen build could collide with ours via the
  dnl process-wide symbol table. Verify with:
  dnl   nm -D --defined-only modules/fastchart.so | grep -v get_module
  dnl Expected: only standard-library symbols (memcpy, etc.) remain.
  FASTCHART_CFLAGS="-Wall -Wextra -Wno-unused-parameter -fvisibility=hidden"

  if test "$PHP_FASTCHART_DEV" = "yes"; then
    FASTCHART_CFLAGS="$FASTCHART_CFLAGS -Werror -Wstrict-prototypes"
  fi

  PHP_ADD_INCLUDE([$ext_srcdir])
  dnl Vendored nayuki QR encoder lives under vendor/qrcodegen/. Adding
  dnl its parent dir to the include path lets fastchart_qrcode.c (and
  dnl any future Symbol class needing the encoder) say
  dnl `#include "qrcodegen.h"` without a relative path. Uses
  dnl $abs_srcdir (autoconf-standard, always populated before this
  dnl macro fires) rather than $ext_srcdir, which is unset at this
  dnl point in m4 expansion order and would resolve to /vendor/qrcodegen.
  PHP_ADD_INCLUDE([$abs_srcdir/vendor/qrcodegen])
  dnl PHP_NEW_EXTENSION will compile vendor/qrcodegen/qrcodegen.c into
  dnl vendor/qrcodegen/qrcodegen.lo — for VPATH / out-of-tree builds the
  dnl matching directory must exist under $ext_builddir or libtool will
  dnl fail to write the .lo file. Mirrors ext/fileinfo's libmagic wiring.
  PHP_ADD_BUILD_DIR([$ext_builddir/vendor/qrcodegen])

  PHP_NEW_EXTENSION(fastchart,
    $WRAPPER_SOURCES,
    $ext_shared,,
    $FASTCHART_CFLAGS)
fi
