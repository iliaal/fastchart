# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Initial scaffold. Builds against PHP 8.3+, depends on `ext/gd`,
  links libgd directly for drawing primitives.
- Public OO surface registered: `FastChart\Chart` (abstract base),
  `FastChart\LineChart`, `BarChart`, `PieChart`, `ScatterChart`,
  `StockChart`. Fluent setters (`setSize`, `setTitle`, `setTheme`,
  per-type `setSeries` / `setSlices` / `setPoints` / `setOhlcv`,
  `setMovingAverages`, `setVolumePane`, `setStacked`,
  `setDonutHoleRatio`).
- `Chart::draw(\GdImage)` validates the canvas via
  `php_gd_libgdimageptr_from_zval_p` and round-trips it. Per-type
  drawing implementations land in subsequent commits.
- `Chart::version()` static returning `PHP_FASTCHART_VERSION`.

[Unreleased]: https://github.com/iliaal/fastchart/commits/master
