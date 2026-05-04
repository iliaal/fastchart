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
  dnl gdImageFilledRectangle, gdImageFTBBox, etc.) on the GdImagePtr we pull
  dnl out of caller-supplied \GdImage zvals. The dynamic linker dedupes
  dnl libgd.so.3 so ext/gd and fastchart share one copy in the address space.
  PHP_CHECK_LIBRARY(gd, gdImageCreateTrueColor,
  [
    PHP_ADD_LIBRARY(gd, 1, FASTCHART_SHARED_LIBADD)
    AC_DEFINE(HAVE_LIBGD, 1, [Have libgd])
  ],[
    AC_MSG_ERROR([libgd not found. Install libgd-dev (Debian/Ubuntu) or gd-devel (RHEL).])
  ])

  PHP_SUBST(FASTCHART_SHARED_LIBADD)

  dnl ext/gd must load before fastchart so php_gd_libgdimageptr_from_zval_p
  dnl is resolvable at fastchart's MINIT and \GdImage class entries are
  dnl registered before any FastChart method dereferences a passed canvas.
  PHP_ADD_EXTENSION_DEP(fastchart, gd)

  WRAPPER_SOURCES="fastchart.c"

  dnl -Wall -Wextra are on by default so wrapper regressions get caught
  dnl in every local build; --enable-fastchart-dev upgrades warnings to
  dnl -Werror plus extra strictness.
  FASTCHART_CFLAGS="-Wall -Wextra -Wno-unused-parameter"

  if test "$PHP_FASTCHART_DEV" = "yes"; then
    FASTCHART_CFLAGS="$FASTCHART_CFLAGS -Werror -Wstrict-prototypes"
  fi

  PHP_NEW_EXTENSION(fastchart,
    $WRAPPER_SOURCES,
    $ext_shared,,
    $FASTCHART_CFLAGS)
fi
