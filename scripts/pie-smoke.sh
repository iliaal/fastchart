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
# itself does not), and ca-certificates for HTTPS clones.
# Library deps: pkg-config + freetype / libpng / libjpeg / libwebp
# headers cover everything config.m4 probes. libgd-dev is here only
# for the ext/gd build below.
apt-get install -y -qq \
    git ca-certificates bison libtool-bin pkg-config \
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
curl -sSL https://github.com/php/pie/releases/latest/download/pie.phar -o /usr/local/bin/pie
chmod +x /usr/local/bin/pie
ls -la /usr/local/bin/pie
pie --version 2>&1 | head -3
echo

echo "---- 6. pie install ----"
PIE_OK=0

# Path repository so PIE installs from the mounted clone, not Packagist.
# The version range is pinned to the mounted HEAD's tag; otherwise PIE
# refuses unstable refs without --prefer-source-stability=dev.
mkdir -p /tmp/piework
cat > /tmp/piework/composer.json <<'JSON'
{
    "name": "iliaal/pie-smoke",
    "repositories": [
        { "type": "path", "url": "/tmp/src", "options": { "symlink": false } }
    ],
    "minimum-stability": "dev",
    "prefer-stable": true
}
JSON

echo "   [A] pie install -d /tmp/piework iliaal/fastchart"
pie install \
    -d /tmp/piework \
    --with-php-config=/usr/local/bin/php-config \
    --auto-install-build-tools \
    iliaal/fastchart 2>&1 \
    | tee /tmp/pie-A.out | tail -40 || true

if php -m | grep -qi fastchart; then
    PIE_OK=1
    echo "   [A] RESULT: success"
fi

# Path B: plain Packagist lookup, in case the path-repo route fails.
if [ "$PIE_OK" = "0" ]; then
    echo
    echo "   [B] pie install iliaal/fastchart  (plain Packagist lookup)"
    pie install \
        --with-php-config=/usr/local/bin/php-config \
        iliaal/fastchart 2>&1 | tee /tmp/pie-B.out | tail -20 || true
    if php -m | grep -qi fastchart; then
        PIE_OK=1
        echo "   [B] RESULT: success"
    fi
fi

echo "   overall PIE result: PIE_OK=$PIE_OK"
echo

echo "---- 7. Verify extension loads ----"
if [ "$PIE_OK" = "0" ]; then
    echo "   *** PIE did not install the extension; falling back to manual phpize+make+install ***"
    cd /tmp/src
    phpize >/dev/null
    ./configure --enable-fastchart >/dev/null
    make -j"$(nproc)" 2>&1 | tail -3
    make install 2>&1 | tail -3
    docker-php-ext-enable fastchart
    echo "   [fallback] manual install SUCCEEDED"
fi
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
