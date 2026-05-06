# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/iliaal/fastchart/compare/0.1.1...HEAD
[0.1.1]: https://github.com/iliaal/fastchart/releases/tag/0.1.1
[0.1.0]: https://github.com/iliaal/fastchart/releases/tag/0.1.0
