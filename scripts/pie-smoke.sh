#!/usr/bin/env bash
set -euo pipefail

# PIE install smoke test for iliaal/fastchart.
#
# Runs inside a php:8.x-cli container with the repo mounted at /fastchart:
#
#   docker run --rm -v "$PWD":/fastchart -w /fastchart \
#     php:8.4-cli ./scripts/pie-smoke.sh
#
# Validates that source builds via PIE work on any host where the
# pkg-config dev packages (freetype / libpng / libjpeg / libwebp) are
# reachable. fastchart 1.0 does not link libgd; ext/gd is loaded only
# so test-side image validation (and this smoke's getimagesize calls)
# can decode raster output. The container build below installs it
# alongside fastchart.

echo "======================================================================"
echo " PIE install smoke test for iliaal/fastchart"
echo "======================================================================"
echo
echo "PHP:"
php --version | head -1
echo "phpize:"
phpize --version 2>&1 | head -2
echo

echo "---- 1. System build tools + pkg-config deps ----"
apt-get update -qq >/dev/null
# Build tools: PIE needs git (clones source via git clone), bison +
# libtoolize (PIE's build-tools check requires both even though phpize
# itself does not), and ca-certificates for HTTPS clones. `unzip` is
# load-bearing — composer shells out to /usr/bin/unzip when extracting
# the prebuilt-binary zip PIE sets via setDistUrl(); if missing,
# composer silently falls back to PHP's ZipArchive which lays out the
# file at a path PIE's prePackagedBinary check doesn't look at, and the
# install fails with ExtensionBinaryNotFound even though the zip
# downloaded fine. `php:8.x-cli` Debian images do not ship unzip.
# Library deps: pkg-config + freetype / libpng / libjpeg / libwebp
# headers cover everything config.m4 probes. libgd-dev is here only
# for the ext/gd build below.
apt-get install -y -qq \
    git ca-certificates bison libtool-bin pkg-config unzip \
    libfreetype6-dev libpng-dev libjpeg-dev libwebp-dev libgd-dev >/dev/null
git --version
bison --version | head -1
libtoolize --version | head -1 || echo "libtoolize not found"
pkg-config --modversion freetype2 libpng libjpeg libwebp || echo "pkg-config probe failed"
echo

echo "---- 2. Build and enable ext/gd (tests need it for image decode) ----"
docker-php-ext-configure gd --with-freetype --with-jpeg --with-webp >/dev/null
docker-php-ext-install -j"$(nproc)" gd >/dev/null
docker-php-ext-enable gd
php -m | grep -i '^gd$'
php -r 'echo "gd version: ", phpversion("gd"), PHP_EOL;'
echo

echo "---- 3. Fresh clone from mounted source (avoids host build artifacts) ----"
git config --global --add safe.directory /fastchart
git config --global --add safe.directory /fastchart/.git
git clone -q file:///fastchart /tmp/src
cd /tmp/src
echo "HEAD: $(git log --oneline -1)"
echo "tag:  $(git describe --tags --always)"
ls composer.json config.m4 php_fastchart.h | head
echo

echo "---- 4. Install Composer ----"
curl -sS https://getcomposer.org/installer | php -- --quiet
mv composer.phar /usr/local/bin/composer
composer --version | head -1
echo

echo "---- 5. Download PIE ----"
PIE_VERSION=1.4.8
PIE_SHA256=ef9f19c2698334aa8ce8fc458c8cf2a31a2fd6a29230216dbde3422343cf952d
curl -fsSL "https://github.com/php/pie/releases/download/${PIE_VERSION}/pie.phar" -o /usr/local/bin/pie
echo "${PIE_SHA256}  /usr/local/bin/pie" | sha256sum -c -
chmod +x /usr/local/bin/pie
ls -la /usr/local/bin/pie
pie --version 2>&1 | head -3
echo

echo "---- 6. pie install (against Packagist, real-user path) ----"
# Smoke runs from /release-ext after the tag is published, so Packagist
# already serves the new version. This is the canonical user install:
# `pie install iliaal/fastchart` resolves to the freshly-tagged release,
# picks up the prebuilt zip when a matching <php-ver, arch, libc> lane
# exists on the release, and falls back to source-build otherwise.
echo "   pie install iliaal/fastchart"
set +e
pie install \
    --with-php-config=/usr/local/bin/php-config \
    --auto-install-build-tools \
    iliaal/fastchart 2>&1 | tee /tmp/pie.out | tail -25
PIE_RC=${PIPESTATUS[0]}
set -e
if [ "$PIE_RC" -ne 0 ]; then
    echo "   PIE install failed with exit code $PIE_RC"
    exit "$PIE_RC"
fi
if ! php -m | grep -qi '^fastchart$'; then
    echo "   PIE returned success but fastchart is not loaded"
    exit 1
fi
echo "   PIE install: success"
echo

echo "---- 7. Verify extension loads ----"
php -m | grep -i fastchart
php -r 'echo "fastchart version: ", phpversion("fastchart"), PHP_EOL;'
php -r 'echo "FastChart\\Chart::version(): ", FastChart\Chart::version(), PHP_EOL;'
echo

echo "---- 8. Functional smoke test ----"
# Render LineChart / StockChart / PieChart across PNG / JPEG / WebP /
# SVG. Magic bytes plus a non-trivial size threshold confirm the
# raster encoders (plutosvg + libpng / libjpeg-turbo / libwebp) and
# the SVG backend are all wired up.
php -r '
$line = (new FastChart\LineChart(160, 120))
    ->setTitle("smoke")
    ->setSeries([["data" => [1, 2, 3, 4, 5]]]);

$png = $line->renderPng();
if (substr($png, 0, 8) !== "\x89PNG\r\n\x1a\n") { echo "LineChart PNG FAIL\n"; exit(1); }
if (strlen($png) < 200) { echo "LineChart PNG too small\n"; exit(1); }
echo "LineChart PNG OK (", strlen($png), " bytes)\n";

$jpg = $line->renderJpeg();
if (substr($jpg, 0, 3) !== "\xff\xd8\xff") { echo "LineChart JPEG FAIL\n"; exit(1); }
echo "LineChart JPEG OK (", strlen($jpg), " bytes)\n";

$webp = $line->renderWebp();
if (substr($webp, 0, 4) !== "RIFF" || substr($webp, 8, 4) !== "WEBP") { echo "LineChart WebP FAIL\n"; exit(1); }
echo "LineChart WebP OK (", strlen($webp), " bytes)\n";

$svg = $line->renderSvg();
if (strpos($svg, "<svg") === false) { echo "LineChart SVG FAIL\n"; exit(1); }
echo "LineChart SVG OK (", strlen($svg), " bytes)\n";

$ohlc = [];
for ($i = 0; $i < 10; $i++) { $ohlc[] = [$i, 100 + $i, 105 + $i, 95 + $i, 102 + $i, 1000]; }
$stockPng = (new FastChart\StockChart())
    ->setSize(200, 100)
    ->setOhlcv($ohlc)
    ->renderPng();
if (substr($stockPng, 0, 8) !== "\x89PNG\r\n\x1a\n") { echo "StockChart PNG FAIL\n"; exit(1); }
echo "StockChart PNG OK (", strlen($stockPng), " bytes)\n";

foreach ([FastChart\Chart::THEME_LIGHT, FastChart\Chart::THEME_DARK] as $t) {
    $bytes = (new FastChart\PieChart(120, 120))
        ->setTheme($t)
        ->setSlices([["label" => "a", "value" => 1], ["label" => "b", "value" => 2]])
        ->renderPng();
    if (substr($bytes, 0, 8) !== "\x89PNG\r\n\x1a\n") { echo "PieChart theme=$t FAIL\n"; exit(1); }
}
echo "PieChart themes OK\n";
'
echo
echo "======================================================================"
echo " PIE install smoke test: PASSED"
echo "======================================================================"
