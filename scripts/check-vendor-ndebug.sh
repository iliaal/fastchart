#!/usr/bin/env bash
# Fails when a vendored object file still calls assert(). FASTCHART_VENDOR_CFLAGS
# carries -DNDEBUG so a vendored debug check cannot abort() a process that is
# serving other requests (every request under a threaded SAPI).
#
# Usage: scripts/check-vendor-ndebug.sh [build-dir]   (default: .)
# Run after make. Exits 2 when no vendored objects are found.
set -euo pipefail

root=${1:-.}
mapfile -t objs < <(find "$root/vendor" -name '*.o' -print | sort)
if [ "${#objs[@]}" -eq 0 ]; then
	echo "no vendored object files under $root/vendor; build first" >&2
	exit 2
fi

bad=0
for obj in "${objs[@]}"; do
	if nm --undefined-only "$obj" | grep -qwE '__assert_fail|__assert_rtn|__assert'; then
		echo "assert() compiled in: $obj"
		bad=1
	fi
done
if [ "$bad" -ne 0 ]; then
	echo "vendored code must build with -DNDEBUG (FASTCHART_VENDOR_CFLAGS in config.m4)" >&2
	exit 1
fi
echo "vendored objects: ${#objs[@]} checked, none call assert()"
