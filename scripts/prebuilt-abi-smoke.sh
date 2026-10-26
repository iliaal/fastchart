#!/usr/bin/env bash
# Usage: prebuilt-abi-smoke.sh MODULE.so OLDEST_IMAGE NEWEST_IMAGE
#
# Gates a prebuilt Linux module against the oldest and newest patch release
# of its PHP minor. Each image is an official php:X.Y.Z-cli image reference;
# the X.Y.Z in its tag is the PHP_VERSION the image must report.
#
# 1. Every undefined (non-weak) dynamic symbol of the module must be exported
#    by the oldest image's php binary or by a library the module itself links
#    against, as resolved inside that image.
# 2. The module must load and run prebuilt-abi-smoke.php on both images.
set -Eeuo pipefail
shopt -s inherit_errexit

if [[ $# -ne 3 ]]; then
	echo "usage: $0 MODULE.so OLDEST_IMAGE NEWEST_IMAGE" >&2
	exit 2
fi

module=$(realpath "$1")
oldest=$2
newest=$3
smoke_php=$(realpath "$(dirname "$0")/prebuilt-abi-smoke.php")

image_version() {
	local ref=${1%%@*}
	ref=${ref##*:}
	printf '%s\n' "${ref%%-*}"
}

work=$(mktemp -d)
cid=""
cleanup() {
	if [[ -n "$cid" ]]; then
		docker rm -f "$cid" >/dev/null 2>&1 || true
	fi
	rm -rf "$work"
}
trap cleanup EXIT

echo "== $(basename "$module"): import check against $oldest"
cid=$(docker create "$oldest")
mkdir -p "$work/providers"
docker cp -L "$cid:/usr/local/bin/php" "$work/providers/php"

ldd_out=$(docker run --rm -v "$module:/ext/module.so:ro" "$oldest" ldd /ext/module.so)
printf '%s\n' "$ldd_out"
mapfile -t libs < <(awk '$2 == "=>" && $3 ~ /^\// { print $3 } $1 ~ /^\// { print $1 }' <<< "$ldd_out")
n=0
for lib in "${libs[@]}"; do
	n=$((n + 1))
	docker cp -L "$cid:$lib" "$work/providers/lib$n"
done

for provider in "$work"/providers/*; do
	nm -D --defined-only "$provider" | awk 'NF >= 3 { sub(/@.*/, "", $3); print $3 }'
done | sort -u > "$work/exported"

nm -D --undefined-only "$module" | awk '$1 == "U" { sub(/@.*/, "", $2); print $2 }' | sort -u > "$work/imported"

if [[ ! -s "$work/exported" ]]; then
	echo "::error::no exported symbols read from $oldest; refusing to pass the import check"
	exit 1
fi

missing=$(comm -23 "$work/imported" "$work/exported")
imported_count=$(wc -l < "$work/imported")
if [[ -n "$missing" ]]; then
	echo "::error::$(basename "$module") imports symbols that $oldest does not export:"
	printf '%s\n' "$missing"
	exit 1
fi
echo "import check OK: $imported_count imports, all exported by $oldest"

status=0
for image in "$oldest" "$newest"; do
	want=$(image_version "$image")
	echo "== $(basename "$module"): load and smoke on $image (PHP $want)"
	set +e
	out=$(docker run --rm \
		-v "$module:/ext/fastchart.so:ro" \
		-v "$smoke_php:/smoke.php:ro" \
		"$image" php -n -d extension=/ext/fastchart.so /smoke.php 2>&1)
	rc=$?
	set -e
	printf '%s\n' "$out"
	if [[ $rc -ne 0 ]]; then
		echo "::error::smoke on $image exited with status $rc"
		status=1
	elif grep -qi 'undefined symbol' <<< "$out"; then
		echo "::error::smoke on $image reported an undefined symbol"
		status=1
	elif ! grep -qF "ok php=$want " <<< "$out"; then
		echo "::error::smoke on $image did not report PHP $want"
		status=1
	fi
done
exit "$status"
