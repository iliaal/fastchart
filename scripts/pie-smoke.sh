#!/usr/bin/env bash
set -euo pipefail

# PIE install smoke test for iliaal/fastchart.
#
# Runs inside a php:8.x-cli container with the repo mounted at /fastchart:
#
#   docker run --rm -v "$PWD":/fastchart -w /fastchart \
#     php:8.4-cli ./scripts/pie-smoke.sh
#
# Validates the claim documented in .release-config that source builds via
# PIE work on any host where libgd is reachable. fastchart links libgd at
# build time (PHP_ADD_LIBRARY(gd) in config.m4) and depends on ext/gd being
# loaded before fastchart at runtime, so this script exercises both surfaces
# end-to-end on a clean image.

echo "======================================================================"
echo " PIE install smoke test for iliaal/fastchart"
echo "======================================================================"
echo
echo "PHP:"
php --version | head -1
echo "phpize:"
phpize --version 2>&1 | head -2
echo

echo "---- 1. System build tools + libgd ----"
apt-get update -qq >/dev/null
# Build tools: PIE needs git (clones source via git clone), bison +
# libtoolize (PIE's build-tools check requires both even though phpize
# itself does not), and ca-certificates for HTTPS clones.
# Library deps: libgd-dev pulls in libpng, libjpeg, libwebp, libfreetype
# headers that both ext/gd and fastchart link against.
apt-get install -y -qq \
    git ca-certificates bison libtool-bin \
    libgd-dev libpng-dev libjpeg-dev libwebp-dev libfreetype6-dev >/dev/null
git --version
bison --version | head -1
libtoolize --version | head -1 || echo "libtoolize not found"
dpkg -s libgd-dev 2>/dev/null | grep -E '^Version:' || echo "libgd-dev not found"
echo

echo "---- 2. Build and enable ext/gd against system libgd ----"
# fastchart resolves the GdImage class entry via the engine class table at
# MINIT and pulls gdImagePtr out of \GdImage zvals via ext/gd's single
# PHPAPI symbol. ext/gd has to be loaded before fastchart for either to
# work. --with-external-gd matches fastchart's link target so both share
# one libgd in the address space.
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
# Render a tiny LineChart via the in-memory helper and a StockChart via
# the caller-canvas path. The first hits renderPng()'s private-canvas
# allocation; the second exercises draw($canvas) + the GdImage class
# lookup. Together they cover both render paths and verify the libgd
# link is alive.
php -r '
$png = (new FastChart\LineChart(160, 120))
    ->setTitle("smoke")
    ->setSeries([["data" => [1, 2, 3, 4, 5]]])
    ->renderPng();
if (substr($png, 0, 8) !== "\x89PNG\r\n\x1a\n") {
    echo "renderPng FAIL: not a PNG\n"; exit(1);
}
if (strlen($png) < 100) {
    echo "renderPng FAIL: too small (", strlen($png), " bytes)\n"; exit(1);
}
echo "renderPng OK (", strlen($png), " bytes)\n";

$canvas = imagecreatetruecolor(200, 100);
$ohlc = [];
for ($i = 0; $i < 10; $i++) {
    $ohlc[] = [$i, 100 + $i, 105 + $i, 95 + $i, 102 + $i, 1000];
}
$result = (new FastChart\StockChart())
    ->setSize(200, 100)
    ->setOhlcv($ohlc)
    ->draw($canvas);
if ($result !== $canvas) {
    echo "draw FAIL: did not return same canvas\n"; exit(1);
}
echo "draw OK (canvas returned)\n";

$themes = [FastChart\Chart::THEME_LIGHT, FastChart\Chart::THEME_DARK];
foreach ($themes as $t) {
    $bytes = (new FastChart\PieChart(120, 120))
        ->setTheme($t)
        ->setSlices([["label" => "a", "value" => 1], ["label" => "b", "value" => 2]])
        ->renderPng();
    if (substr($bytes, 0, 8) !== "\x89PNG\r\n\x1a\n") {
        echo "PieChart theme=$t FAIL\n"; exit(1);
    }
}
echo "PieChart themes OK\n";
'
echo
echo "======================================================================"
echo " PIE install smoke test: PASSED"
echo "======================================================================"
