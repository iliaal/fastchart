#!/usr/bin/env bash
# Usage: build-linux-php.sh PHP_VERSION PHP_SHA256
#
# Builds a release-mode NTS PHP from the checksum-pinned php.net tarball into
# /usr/local of a Debian base image, so build-linux-prebuilt.sh and
# prebuilt-abi-smoke.sh can treat the committed container like an official
# php:<version>-cli image. Used for minors whose official images are ZTS-only
# (8.6 onward). Runs as root in the container; PHPIZE_DEPS comes from the
# environment, matching the official images' variable.
set -Eeuo pipefail

if [[ $# -ne 2 ]]; then
	echo "usage: $0 PHP_VERSION PHP_SHA256" >&2
	exit 2
fi
version=$1
sha256=$2
: "${PHPIZE_DEPS:?}"

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
# shellcheck disable=SC2086
apt-get install -y -qq --no-install-recommends ca-certificates curl xz-utils $PHPIZE_DEPS >/dev/null

src=/usr/src/php
mkdir -p "$src"
curl -fsSL --retry 5 --retry-all-errors --retry-delay 10 \
	-o /usr/src/php.tar.xz "https://www.php.net/distributions/php-${version}.tar.xz"
echo "${sha256}  /usr/src/php.tar.xz" | sha256sum -c -
tar -xJf /usr/src/php.tar.xz -C "$src" --strip-components=1
cd "$src"
./configure --prefix=/usr/local --disable-all --enable-cli \
	--disable-cgi --disable-phpdbg --without-pear --disable-zts --disable-debug \
	>/tmp/php-configure.log
make -j"$(nproc)" >/tmp/php-make.log 2>&1 || { tail -50 /tmp/php-make.log; exit 1; }
make install >/dev/null
cd /
rm -rf "$src" /usr/src/php.tar.xz

have=$(/usr/local/bin/php -n -r 'echo PHP_VERSION, " zts=", (int) PHP_ZTS, " debug=", (int) PHP_DEBUG;')
if [[ $have != "$version zts=0 debug=0" ]]; then
	echo "::error::built PHP reports $have, expected $version zts=0 debug=0" >&2
	exit 1
fi
echo "built PHP $have"
