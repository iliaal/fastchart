#!/usr/bin/env bash
# Regression check for GSHUTDOWN under ZTS: module shutdown destroys the
# globals of worker threads that never called ts_free_thread() from the main
# thread (ts_free_id), so GSHUTDOWN must release the globals it is handed,
# not the calling thread's. Each worker renders a chart first so its
# FreeType library and glyph caches are populated when they are released.
#
# Needs a ZTS PHP built with --enable-embed (libphp.so). An ASan build of
# that PHP reports the old bug as a SEGV in fastchart_ft_library_shutdown.
#
# Usage: scripts/zts-embed-gshutdown.sh PHP_PREFIX [fastchart.so] [font.ttf]
set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
prefix=${1:?usage: $0 PHP_PREFIX [fastchart.so] [font.ttf]}
so=$(realpath "${2:-$here/../modules/fastchart.so}")
font=${3:-/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf}
php_config=$prefix/bin/php-config

[ -f "$so" ] || { echo "missing $so; build first" >&2; exit 2; }
[ -f "$font" ] || { echo "missing font $font" >&2; exit 2; }
[ -e "$prefix/lib/libphp.so" ] || { echo "$prefix has no libphp.so (needs --enable-embed)" >&2; exit 2; }
if ! grep -q -- '--enable-zts' <<<"$("$php_config" --configure-options)"; then
	echo "$prefix is not a ZTS build" >&2
	exit 2
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cflags=$("$php_config" --includes)
undef=$(nm -D --undefined-only "$prefix/lib/libphp.so")
if grep -qw __asan_init <<<"$undef"; then
	cflags="$cflags -fsanitize=address"
fi
# shellcheck disable=SC2086
cc -O1 -g $cflags -o "$work/driver" "$here/zts-embed-driver.c" \
	-L"$prefix/lib" -Wl,-rpath,"$prefix/lib" -lphp -lpthread

script="\$c = (new FastChart\\BarChart(300, 200))->setFontPath('$font')
	->setTitle('t')->setSeries([1, 2, 3]);
	if (strlen(\$c->renderPng()) < 100) { echo \"BAD\\n\"; }"

status=0
for free_thread in 0 1; do
	log=$work/run-$free_thread.log
	# Workers left unfreed (free_thread=0) strand every other extension's
	# per-thread globals, which LeakSanitizer would report; the check here
	# is the shutdown crash, not those leaks.
	rc=0
	ASAN_OPTIONS=detect_leaks=0:abort_on_error=0 \
		"$work/driver" "$so" 4 3 "$free_thread" "$script" >"$log" 2>&1 || rc=$?
	if [ "$rc" -ne 0 ] || ! grep -q '^driver done' "$log" \
		|| grep -qE 'AddressSanitizer|BAD' "$log"; then
		echo "FAIL free_thread=$free_thread (exit $rc)"
		cat "$log"
		status=1
	else
		echo "PASS free_thread=$free_thread"
	fi
done
exit "$status"
