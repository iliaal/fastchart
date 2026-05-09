# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.2.0] - 2026-05-09

### Added
- **Symbol family.** Two-class symbology hierarchy parallel to `Chart`:
  `FastChart\Code128` (1D barcode, ISO/IEC 15417) and
  `FastChart\QrCode` (2D matrix code, ISO/IEC 18004). Render-only API
  (`renderPng()`, `renderJpeg()`, `renderWebp()`, `renderGif()`,
  `renderAvif()`, `renderToFile()`) — Symbol classes do not accept a
  caller-supplied `\GdImage`. `Code128` auto-switches between subsets
  A/B/C with an odd-tail-to-C optimisation; mod-103 checksum is
  appended automatically; `setShowText(true)` renders the human-
  readable payload below the bars. `QrCode` ships with all four ECC
  levels (`ECC_L`/`M`/`Q`/`H`) and versions 1..40 via the vendored
  nayuki QR encoder under `vendor/qrcodegen/`. Both classes honour
  `setSize()`, `setQuietZone()`, `setForeground()`, `setBackground()`,
  `setTransparentBackground()`, and `setDpi()` (with PNG/JPEG metadata
  written via `gdImageSetResolution`).

### Fixed
- `StockChart::setOhlcv()` now clears overlay (Bollinger / SAR) and
  indicator-pane buffers when the new candle count is shorter than
  the previous one. Previously the overlay's `n` field kept the old
  length and the renderer walked off the end of the new candle
  array — a use-after-realloc OOB read on the candle pointer.
- Setters that accept paths (`setFont`, `setBackgroundImage`,
  `addIconAt`) now throw an explicit `Error` before
  `RETURN_THROWS` when `php_check_open_basedir` blocks the path.
  `php_check_open_basedir` only emits `E_WARNING` and does not set
  `EG(exception)`, so `RETURN_THROWS` asserted under a debug PHP
  build.
- `QrCode::setQuietZone()` rejects quiet zones above 256 modules
  with a `ValueError` instead of silently clamping.

### Changed
- `composer.json` license expression is now
  `(BSD-3-Clause AND MIT)`. fastchart's own code is unchanged
  BSD-3-Clause; the composite expression declares the MIT-licensed
  nayuki QR encoder vendored under `vendor/qrcodegen/`.

## [0.1.1] - 2026-05-06

### Fixed
- Module load order: declare `ZEND_MOD_REQUIRED("gd")` on the module
  entry so the engine reorders MINIT regardless of `php.ini` / conf.d /
  `-d extension=` order. Previously, configurations that loaded
  `fastchart` before `gd` (notably `docker-php-ext-enable`'s
  alphabetical `conf.d/*.ini`) failed startup with "GdImage class not
  found." `PHP_ADD_EXTENSION_DEP` in `config.m4` is build-system-only
  and does not affect runtime ordering.

### Added
- `scripts/pie-smoke.sh`: PIE install + functional smoke test that
  runs against `php:8.x-cli` with `libgd-dev` and `ext/gd` provisioned
  in the container. Wired into `.release-config` as `smoke_test`.

## [0.1.0] - 2026-05-06

### Added
- Initial public release of fastchart.

[Unreleased]: https://github.com/iliaal/fastchart/compare/0.2.0...HEAD
[0.2.0]: https://github.com/iliaal/fastchart/releases/tag/0.2.0
[0.1.1]: https://github.com/iliaal/fastchart/releases/tag/0.1.1
[0.1.0]: https://github.com/iliaal/fastchart/releases/tag/0.1.0
