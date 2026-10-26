#!/usr/bin/env bash
# Usage: build-linux-prebuilt.sh PHP_VERSION
#
# Builds the Linux prebuilt module from the source root inside an official
# php:<PHP_VERSION>-cli image: the four codec libraries as static PIC
# archives, then fastchart.so linked against them. Runs as root in the
# container. The codec versions and tarball SHA-256s come from the
# environment (FT_*, PNG_*, JPEG_*, WEBP_*), pinned in release-linux.yml.
# Building in the image also fixes the binary's glibc floor at the image's.
set -Eeuo pipefail

if [[ $# -ne 1 ]]; then
	echo "usage: $0 PHP_VERSION" >&2
	exit 2
fi
want=$1
: "${FT_VERSION:?}" "${FT_SHA256:?}" "${PNG_VERSION:?}" "${PNG_SHA256:?}"
: "${JPEG_VERSION:?}" "${JPEG_SHA256:?}" "${WEBP_VERSION:?}" "${WEBP_SHA256:?}"

have=$(php-config --version)
if [[ $have != "$want" ]]; then
	echo "::error::build image has PHP $have, expected $want" >&2
	exit 1
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
# PHPIZE_DEPS is set by the official image. zlib is the only codec
# dependency linked dynamically: libz.so.1 has been ABI-stable for decades.
# shellcheck disable=SC2086
apt-get install -y -qq --no-install-recommends $PHPIZE_DEPS cmake nasm zlib1g-dev >/dev/null

src_root=$PWD
PREFIX=/opt/static-deps
mkdir -p "$PREFIX" /tmp/codec-src
cd /tmp/codec-src
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
# CDN mirrors (savannah/sourceforge) intermittently 502; retry instead of
# failing the release lane. Integrity is still enforced by the per-tarball
# sha256 check after each fetch.
CURL_RETRY=(--retry 5 --retry-all-errors --retry-delay 10)

# --- FreeType ---
# Disable every optional integration (zlib/bz2/png/brotli/harfbuzz) so the
# static archive links only against libc. PIC is mandatory for the .a to
# be linked into the .so.
curl -fsSL "${CURL_RETRY[@]}" -o ft.tar.xz "https://download.savannah.gnu.org/releases/freetype/freetype-${FT_VERSION}.tar.xz"
echo "${FT_SHA256}  ft.tar.xz" | sha256sum -c -
tar xf ft.tar.xz && cd "freetype-${FT_VERSION}"
./configure --prefix="$PREFIX" --enable-static --disable-shared \
	--with-zlib=no --with-bzip2=no --with-png=no --with-brotli=no \
	--with-harfbuzz=no CFLAGS=-fPIC
make -j"$(nproc)" install
cd ..

# --- libpng (linked against system zlib) ---
curl -fsSL "${CURL_RETRY[@]}" -o png.tar.gz "https://download.sourceforge.net/libpng/libpng-${PNG_VERSION}.tar.gz"
echo "${PNG_SHA256}  png.tar.gz" | sha256sum -c -
tar xf png.tar.gz && cd "libpng-${PNG_VERSION}"
./configure --prefix="$PREFIX" --enable-static --disable-shared CFLAGS=-fPIC
make -j"$(nproc)" install
cd ..

# --- libjpeg-turbo (cmake; ENABLE_SHARED=0 produces libjpeg.a) ---
curl -fsSL "${CURL_RETRY[@]}" -o jpeg.tar.gz "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/${JPEG_VERSION}/libjpeg-turbo-${JPEG_VERSION}.tar.gz"
echo "${JPEG_SHA256}  jpeg.tar.gz" | sha256sum -c -
tar xf jpeg.tar.gz && cd "libjpeg-turbo-${JPEG_VERSION}"
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX="$PREFIX" \
	-DCMAKE_INSTALL_LIBDIR=lib \
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	-DENABLE_SHARED=0 -DENABLE_STATIC=1 -DWITH_TURBOJPEG=0 ..
make -j"$(nproc)" install
cd ../..

# --- libwebp (static, no demux/mux extras) ---
curl -fsSL "${CURL_RETRY[@]}" -o webp.tar.gz "https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-${WEBP_VERSION}.tar.gz"
echo "${WEBP_SHA256}  webp.tar.gz" | sha256sum -c -
tar xf webp.tar.gz && cd "libwebp-${WEBP_VERSION}"
./configure --prefix="$PREFIX" --enable-static --disable-shared \
	--disable-libwebpmux --disable-libwebpdemux --disable-libwebpextras \
	CFLAGS=-fPIC
make -j"$(nproc)" install
cd ..

echo "Static prefix contents:"
find "$PREFIX/lib" -maxdepth 2 \( -name '*.a' -o -name '*.pc' \) -print | head -20

cd "$src_root"
phpize
./configure --enable-fastchart --enable-fastchart-dev \
	--with-fastchart-static-codecs="$PREFIX"
make -j"$(nproc)"
find modules -maxdepth 1 -type f -print
