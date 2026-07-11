dnl config.m4 for extension fastchart

PHP_ARG_ENABLE(fastchart, whether to enable fastchart support,
[  --enable-fastchart      Enable fastchart support])

PHP_ARG_ENABLE(fastchart-dev, whether to enable developer build flags,
[  --enable-fastchart-dev  Upgrade wrapper warnings to -Werror plus strict checks], no, no)

PHP_ARG_WITH(fastchart-static-codecs, prefix containing static-built codec libs,
[  --with-fastchart-static-codecs[=PREFIX]
                          Link freetype / libpng / libjpeg-turbo / libwebp
                          statically from .a archives in PREFIX/lib. Set by
                          the Linux prebuilt-binary release lane so the
                          produced fastchart.so has no codec-lib NEEDED
                          entries and works on both Ubuntu and Debian
                          (which disagree on libjpeg soname major).
                          PREFIX of "yes" uses whatever PKG_CONFIG_PATH
                          already resolves.], no, no)

PHP_ARG_WITH(pdfio, whether to enable PDF output via system pdfio,
[  --with-pdfio[=PREFIX]   Enable renderPdf() / renderToFile('*.pdf') using a
                          system-installed pdfio (msweet.org). Off by default;
                          PDF methods throw "PDF support not compiled in" when
                          absent. Vector output — charts emit PDF path operators
                          directly, no rasterization. PREFIX of "yes" uses
                          whatever PKG_CONFIG_PATH already resolves.], no, no)

if test "$PHP_FASTCHART" != "no"; then

  PHP_VERSION_ID=$($PHP_CONFIG --vernum)
  if test "$PHP_VERSION_ID" -lt "80100"; then
    AC_MSG_ERROR([fastchart requires PHP 8.1.0 or later (found $PHP_VERSION_ID)])
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

  dnl Static-codec-libs mode: prepend the static-prefix to
  dnl PKG_CONFIG_PATH and switch every pkg-config invocation to
  dnl --static. The --static flag emits transitive Requires.private
  dnl deps onto the link line; in dynamic mode those deps were
  dnl already pulled in via the shared libs' NEEDED entries so
  dnl adding them again is harmless. In static mode they're the
  dnl only way the linker finds the transitive .a archives.
  dnl
  dnl Also force --exclude-libs=ALL so the libjpeg / libpng / libwebp
  dnl / libfreetype symbols pulled in from .a archives are localized
  dnl in the produced fastchart.so rather than exported globally.
  dnl Without this, loading ext/gd (which dynamically links the
  dnl system libjpeg.so.8) alongside our statically-linked
  dnl libjpeg-turbo 3.x in the same process collides on the
  dnl jpeg_CreateCompress / jpeg_destroy / etc. symbol table —
  dnl callers inside fastchart.so end up resolving against
  dnl ext/gd's older libjpeg's symbol versions and JPEG_LIB_VERSION
  dnl validation rejects the call ("encoder produced no output").
  dnl -fvisibility=hidden in CFLAGS doesn't cover static-archive
  dnl symbols; --exclude-libs is the link-time equivalent.
  FC_PKGCFG_STATIC=""
  if test "$PHP_FASTCHART_STATIC_CODECS" != "no"; then
    if test "$PHP_FASTCHART_STATIC_CODECS" != "yes"; then
      PKG_CONFIG_PATH="$PHP_FASTCHART_STATIC_CODECS/lib/pkgconfig:$PKG_CONFIG_PATH"
      export PKG_CONFIG_PATH
    fi
    FC_PKGCFG_STATIC="--static"
    AC_MSG_NOTICE([fastchart: static-codec-libs mode, PKG_CONFIG_PATH=$PKG_CONFIG_PATH])
  fi

  if ! $FC_PKGCFG --exists freetype2; then
    AC_MSG_ERROR([pkg-config freetype2 not found. Install libfreetype-dev / freetype-devel — FreeType is required for text rendering.])
  fi

  FC_PC_CFLAGS=`$FC_PKGCFG $FC_PKGCFG_STATIC --cflags freetype2`
  FC_PC_LIBS=`$FC_PKGCFG $FC_PKGCFG_STATIC --libs   freetype2`
  FC_OPT_DEFS=""

  for fc_pair in libpng:HAVE_LIBPNG libjpeg:HAVE_LIBJPEG libwebp:HAVE_LIBWEBP; do
    fc_lib=`echo $fc_pair | cut -d: -f1`
    fc_def=`echo $fc_pair | cut -d: -f2`
    if $FC_PKGCFG --exists $fc_lib; then
      FC_PC_CFLAGS="$FC_PC_CFLAGS `$FC_PKGCFG $FC_PKGCFG_STATIC --cflags $fc_lib`"
      FC_PC_LIBS="$FC_PC_LIBS `$FC_PKGCFG $FC_PKGCFG_STATIC --libs $fc_lib`"
      FC_OPT_DEFS="$FC_OPT_DEFS -D$fc_def=1"
      AC_MSG_NOTICE([fastchart: $fc_lib found, $fc_def enabled])
    else
      AC_MSG_NOTICE([fastchart: $fc_lib not found — corresponding output format will be unavailable at runtime])
    fi
  done

  dnl Optional PDF output via system pdfio (msweet.org). Opt-in: unlike
  dnl the codecs above (auto-detected), --with-pdfio must be requested,
  dnl and a request that can't be satisfied is a hard error rather than
  dnl a silent skip — the user explicitly asked for PDF. pdfio is not
  dnl vendored; it carries a PDF parser fastchart never uses, so it
  dnl stays a system dependency. PDF emission is vector (chart bodies
  dnl emit PDF path operators directly through the target abstraction),
  dnl so it needs no codec libs.
  FC_PDF_SRC=""
  if test "$PHP_PDFIO" != "no"; then
    if test "$PHP_PDFIO" != "yes"; then
      PKG_CONFIG_PATH="$PHP_PDFIO/lib/pkgconfig:$PKG_CONFIG_PATH"
      export PKG_CONFIG_PATH
    fi
    if $FC_PKGCFG --exists pdfio; then
      FC_PC_CFLAGS="$FC_PC_CFLAGS `$FC_PKGCFG $FC_PKGCFG_STATIC --cflags pdfio`"
      FC_PC_LIBS="$FC_PC_LIBS `$FC_PKGCFG $FC_PKGCFG_STATIC --libs pdfio`"
      FC_OPT_DEFS="$FC_OPT_DEFS -DHAVE_FASTCHART_PDF=1"
      FC_PDF_SRC="fastchart_pdf.c"
      AC_MSG_NOTICE([fastchart: pdfio found, PDF output enabled])
    else
      AC_MSG_ERROR([--with-pdfio given but pkg-config cannot find pdfio. Install it from https://www.msweet.org/pdfio (make install ships pdfio.pc), or pass --with-pdfio=PREFIX.])
    fi
  fi

  PHP_EVAL_INCLINE([$FC_PC_CFLAGS])
  PHP_EVAL_LIBLINE([$FC_PC_LIBS], FASTCHART_SHARED_LIBADD)

  dnl Localize symbols pulled in from ANY static .a archive so they are
  dnl not re-exported through fastchart.so's dynamic symbol table. Two
  dnl static-archive paths need this: --with-fastchart-static-codecs
  dnl (libjpeg / libpng / libwebp / libfreetype .a) and --with-pdfio
  dnl linked against a static libpdfio.a (which drags in 200+ pdfio*/
  dnl ttf* symbols). PHP dlopens extensions with RTLD_GLOBAL, so any
  dnl re-exported symbol can interpose another extension's copy — e.g.
  dnl our statically-linked libjpeg-turbo 3.x colliding with ext/gd's
  dnl dynamic libjpeg.so.8, where JPEG_LIB_VERSION validation then
  dnl rejects the call ("encoder produced no output"). -fvisibility=
  dnl hidden in CFLAGS can't reach static-archive objects; --exclude-libs
  dnl is the link-time equivalent. It is a no-op for dynamically NEEDED
  dnl libs, so applying it whenever the linker accepts it is safe. GNU ld
  dnl and gold accept it; macOS ld64 does not — hence the link probe.
  FC_SAVE_LDFLAGS="$LDFLAGS"
  LDFLAGS="$LDFLAGS -Wl,--exclude-libs=ALL"
  AC_MSG_CHECKING([whether the linker accepts -Wl,--exclude-libs=ALL])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
    [AC_MSG_RESULT([yes])
     FASTCHART_SHARED_LIBADD="$FASTCHART_SHARED_LIBADD -Wl,--exclude-libs=ALL"],
    [AC_MSG_RESULT([no])])
  LDFLAGS="$FC_SAVE_LDFLAGS"

  PHP_SUBST(FASTCHART_SHARED_LIBADD)

  dnl First-party sources (the repo-root fastchart*.c). These compile
  dnl under the strict, unsuppressed warning set (and -Werror in dev
  dnl builds); the vendored sources below are added separately with the
  dnl warning suppressions they need.
  FASTCHART_SOURCES="fastchart.c \
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
    fastchart_bullet.c \
    fastchart_pareto.c \
    fastchart_calendar.c \
    fastchart_sunburst.c \
    fastchart_sankey.c \
    fastchart_marimekko.c \
    fastchart_vector.c \
    fastchart_graph.c \
    fastchart_arc.c \
    fastchart_chord.c \
    fastchart_network.c \
    fastchart_pyramid.c \
    fastchart_violin.c \
    fastchart_circlepack.c \
    fastchart_pictogram.c \
    fastchart_venn.c \
    fastchart_wordcloud.c \
    fastchart_serpentine.c \
    fastchart_dendrogram.c \
    fastchart_partition.c \
    fastchart_effects.c \
    fastchart_encoder.c \
    fastchart_rasterize.c \
    fastchart_symbol.c \
    fastchart_code128.c \
    fastchart_qrcode.c \
    $FC_PDF_SRC"

  dnl Vendored sources. Kept in a separate list so they can be compiled
  dnl with the warning suppressions they require (see FASTCHART_VENDOR_-
  dnl CFLAGS below) without those suppressions leaking onto first-party
  dnl code — PHP applies one CFLAGS set per PHP_NEW_EXTENSION call.
  FASTCHART_VENDOR_SOURCES="vendor/qrcodegen/qrcodegen.c \
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
  dnl in every local build; --enable-fastchart-dev upgrades first-party
  dnl warnings to -Werror plus extra strictness.
  dnl
  dnl -fvisibility=hidden keeps the vendored plutovg_/plutosvg_/qrcodegen_
  dnl symbols (and every internal fastchart_* helper) out of the dynamic
  dnl symbol table. Only get_module stays exported, marked default by
  dnl ZEND_DLEXPORT. PLUTOVG_BUILD_STATIC + PLUTOSVG_BUILD_STATIC make those
  dnl libraries' headers stop applying visibility=default on their API
  dnl decorations, so -fvisibility=hidden actually buries them. Both
  dnl defines stay on first-party TUs too: fastchart.c / fastchart_-
  dnl rasterize.c / fastchart_target.c include those headers and must
  dnl agree on the (hidden) decoration.
  dnl Verify with:
  dnl   nm -D --defined-only modules/fastchart.so | grep -v get_module
  dnl Expected: only standard-library symbols (memcpy, etc.) remain.
  dnl PLUTOVG_DISABLE_IMAGE_WRITE (opt #12): drops the plutovg_surface_-
  dnl write_to_* (PNG/JPEG file + stream) functions and their stb_image_
  dnl write.h backing. fastchart routes every raster output through
  dnl libpng / libjpeg-turbo / libwebp directly; nothing in fastchart,
  dnl plutosvg, or the phpt suite calls plutovg's write functions, and
  dnl -fvisibility=hidden plus PLUTOVG_BUILD_STATIC already keep them
  dnl out of the dynamic symbol table. Disabling the entire translation
  dnl unit shrinks fastchart.so by ~70 KB and saves ~1 s of incremental
  dnl build time.
  dnl -Wno-unused-parameter stays on first-party TUs: PHP's own headers
  dnl trip it under clang (zend_arena.h's static-inline stubs carry
  dnl unused params in ZEND_DEBUG builds — the ASAN lane), and php-src
  dnl compiles itself with the same suppression. Every other class from
  dnl the vendor suppression set below is enforced on first-party code.
  FASTCHART_CFLAGS="-Wall -Wextra \
    -Wno-unused-parameter \
    -fvisibility=hidden \
    -DPLUTOVG_BUILD_STATIC -DPLUTOSVG_BUILD_STATIC \
    -DPLUTOVG_DISABLE_IMAGE_WRITE \
    $FC_OPT_DEFS"

  dnl Vendor-driven warning suppressions: plutovg ships stb_image* /
  dnl stb_image_write* / stb_truetype headers + ft-stroker code that
  dnl trip a few standard warnings (unused statics, signed/unsigned
  dnl comparisons in tight inner loops, implicit fallthrough in case
  dnl arms). These are scoped to the vendored TUs only — the
  dnl PHP_ADD_SOURCES call below compiles FASTCHART_VENDOR_SOURCES with
  dnl this set, so the suppressions never mask a regression in first-
  dnl party fastchart*.c (which build under the strict FASTCHART_CFLAGS
  dnl above, plus -Werror in dev builds). -Werror is deliberately NOT
  dnl added here: vendored code stays warning-tolerant regardless of how
  dnl complete the suppression list is. The visibility and static-build
  dnl defines are repeated because the vendored TUs need them too.
  FASTCHART_VENDOR_CFLAGS="-Wall -Wextra \
    -Wno-unused-parameter -Wno-unused-function -Wno-sign-compare \
    -Wno-implicit-fallthrough -Wno-unused-but-set-variable \
    -Wno-misleading-indentation -Wno-missing-field-initializers \
    -fvisibility=hidden \
    -DPLUTOVG_BUILD_STATIC -DPLUTOSVG_BUILD_STATIC \
    -DPLUTOVG_DISABLE_IMAGE_WRITE"

  if test "$PHP_FASTCHART_DEV" = "yes"; then
    FASTCHART_CFLAGS="$FASTCHART_CFLAGS -Werror -Wstrict-prototypes"
  fi

  PHP_NEW_EXTENSION(fastchart,
    $FASTCHART_SOURCES,
    $ext_shared,,
    $FASTCHART_CFLAGS)

  dnl Add the vendored sources with their own suppression-carrying flags.
  dnl PHP_NEW_EXTENSION has already emitted the shared-module link rule,
  dnl which references the object list through a make variable resolved at
  dnl build time, so objects appended here still reach the final link.
  dnl Mirror the shared-vs-static object-array choice PHP_NEW_EXTENSION
  dnl makes internally (shared_objects_fastchart for a shared module,
  dnl PHP_GLOBAL_OBJS for a static in-tree build).
  if test "$PHP_FASTCHART_SHARED" = "yes"; then
    PHP_ADD_SOURCES_X($ext_dir, $FASTCHART_VENDOR_SOURCES, $FASTCHART_VENDOR_CFLAGS -DZEND_COMPILE_DL_EXT=1, shared_objects_fastchart, yes)
  else
    PHP_ADD_SOURCES($ext_dir, $FASTCHART_VENDOR_SOURCES, $FASTCHART_VENDOR_CFLAGS)
  fi

  PHP_ADD_INCLUDE([$ext_srcdir])
  PHP_ADD_INCLUDE([$ext_srcdir/vendor/qrcodegen])
  PHP_ADD_INCLUDE([$ext_srcdir/vendor/plutovg/include])
  PHP_ADD_INCLUDE([$ext_srcdir/vendor/plutovg/source])
  PHP_ADD_INCLUDE([$ext_srcdir/vendor/plutosvg/source])

  dnl PHP_NEW_EXTENSION compiles vendor/*/qrcodegen.c, plutovg-*.c, and
  dnl plutosvg.c into matching .lo files — for VPATH builds the directory
  dnl must exist under $ext_builddir or libtool will fail to write.
  PHP_ADD_BUILD_DIR([$ext_builddir/vendor/qrcodegen])
  PHP_ADD_BUILD_DIR([$ext_builddir/vendor/plutovg/source])
  PHP_ADD_BUILD_DIR([$ext_builddir/vendor/plutosvg/source])
fi
