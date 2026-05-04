# fastchart

Native C PHP extension for fast chart generation. Draws line, bar,
pie, scatter, and candlestick (stock) charts onto caller-supplied
[GD](https://www.php.net/manual/en/book.image.php) canvases. The
canvas stays under the user's control: pick dimensions, color depth,
and output format (PNG / JPEG / WebP / AVIF) via the standard
`imagepng()` / `imagejpeg()` / `imagewebp()` / `imageavif()`
functions on the same `\GdImage`.

## Status

Pre-release scaffold. Public OO surface is registered and reachable;
per-type drawing is implemented incrementally on top of this base.

## Build

```sh
phpize
./configure --enable-fastchart
make -j
make test
```

Strict-warnings dev build: `./configure --enable-fastchart --enable-fastchart-dev`.

Runtime check:

```sh
php -d extension=./modules/fastchart.so \
  -r 'echo FastChart\Chart::version(), PHP_EOL;'
```

## Requirements

- PHP 8.3 or later (NTS or ZTS).
- `ext/gd` enabled — fastchart pulls the underlying `gdImagePtr` out
  of caller-supplied `\GdImage` zvals via
  `php_gd_libgdimageptr_from_zval_p()`.
- libgd development headers at build time (`libgd-dev` on Debian /
  Ubuntu, `gd-devel` on RHEL / Fedora).

## Usage

```php
$canvas = imagecreatetruecolor(1200, 600);

(new FastChart\StockChart())
    ->setSize(1200, 600)
    ->setTitle('AAPL — last 90 days')
    ->setTheme(FastChart\Chart::THEME_DARK)
    ->setOhlcv($ohlcvRows)              // [[ts, o, h, l, c, v], …]
    ->setMovingAverages([20, 50, 200])
    ->setVolumePane(true)
    ->draw($canvas);

imagepng($canvas, '/tmp/aapl.png');
```

Each `set*()` method returns `$this` so calls chain. `draw($canvas)`
returns the same `\GdImage` for the same reason.

## Public classes

- `FastChart\Chart` — abstract base. Carries shared geometry / theme
  setters and the `version()` static.
- `FastChart\LineChart`, `FastChart\BarChart`,
  `FastChart\ScatterChart` — series-based plots.
- `FastChart\PieChart` — slice-based, with optional donut hole.
- `FastChart\StockChart` — OHLC(V) candlesticks, moving-average
  overlays, optional volume sub-pane.

## License

PHP License v3.01. See [`LICENSE`](LICENSE).
