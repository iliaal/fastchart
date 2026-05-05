# Contributing to fastchart

## Requirements

- PHP 8.3 or later (debug build recommended for development:
  `--enable-debug`)
- C compiler: GCC 11+, Clang 14+
- `phpize` and `php-config` (from `php-dev` / `php8.x-dev`)
- GNU Make
- libgd development headers (`libgd-dev` on Debian / Ubuntu,
  `gd-devel` on RHEL / Fedora). FreeType comes in as a transitive
  dependency on every Linux distribution.
- ext/gd loaded at runtime (build it once against your PHP install if
  `make install` doesn't deploy it; AGENTS.md has the recipe).

fastchart links libgd dynamically and shares ext/gd's `libgd.so.3`
copy at runtime — there's no vendored library to bump.

## Bug reports

Use the [GitHub issue tracker](https://github.com/iliaal/fastchart/issues).
Include:

- PHP version (`php -v`)
- fastchart version (`php -r 'echo phpversion("fastchart");'`)
- libgd version (`php -r 'echo gd_info()["GD Version"];'`)
- Operating system and compiler version
- Minimal reproducing code: the chart class, the setters you called,
  the data array, and the rendered output (or crash trace) you got
  vs. expected
- Any error messages, exceptions, or sanitiser output

Before filing, try to reproduce against the latest `master` branch.

**For security issues, do not file a public issue.** See
[SECURITY.md](SECURITY.md) for the private reporting process.

## Pull requests

1. Fork and clone the repo.
2. Create a topic branch off `master`.
3. Make your changes.
4. Add or update tests in `tests/` (PHPT format).
5. Build clean and run the full suite:

   ```sh
   ~/php-install-PHP-8.4/bin/phpize
   ./configure --with-php-config=$HOME/php-install-PHP-8.4/bin/php-config \
       --enable-fastchart --enable-fastchart-dev
   make -j$(nproc)

   ASAN_OPTIONS=detect_leaks=0 \
   TEST_PHP_EXECUTABLE=$HOME/php-install-PHP-8.4/bin/php \
   TEST_PHP_ARGS="-d extension=$HOME/php-src-8.4/ext/gd/modules/gd.so \
                  -d extension=$(pwd)/modules/fastchart.so" \
   NO_INTERACTION=1 \
   $HOME/php-install-PHP-8.4/bin/php run-tests.php tests/
   ```

6. Verify zero compiler warnings (`--enable-fastchart-dev` adds
   `-Werror -Wextra -Wstrict-prototypes`; the release matrix treats
   any warning as a build failure) and that all 86+ tests pass.
7. Push and open a PR against `master`.

### Commit message conventions

- Short imperative subject line (≤ 72 chars).
- Body wraps at 72 columns, explains **why** not **what**.
- No `Co-Authored-By` lines. No AI attribution.
- Audit the message against `git show --stat HEAD` before pushing —
  if the subject claims a fix in `fastchart_axis.c`, the diff had
  better show it.

### Test guidelines

- Tests use PHPT format. See existing tests under `tests/` for shape.
- Prefer exact-byte expectations via `--EXPECT--`. Use `--EXPECTF--`
  only when the output legitimately varies (e.g. line numbers in
  exception traces).
- Pixel-count tests (image-region color counts) need realistic
  thresholds: account for thick lines transiting the region, not just
  the swatch / fill the test is targeting. See
  `tests/008_legend.phpt` and `tests/016_legend_position.phpt` for
  the threshold-comment idiom.
- Visual regressions: re-render the relevant `docs/examples/*.png`
  via the example scripts and eyeball the diff before opening the
  PR. The gallery is the de-facto visual baseline.

### Code style

- Tabs for indentation, K&R braces, follows the wider PHP-src
  convention.
- Comments only when the *why* is non-obvious. Implementation detail
  doesn't need a comment; an external-API constraint (e.g. libgd's
  thick-line vs anti-aliased mutex) does.
- File headers carry the BSD 3-Clause boxed comment plus an
  `| Author: ` line. New files credit yourself there too.
- Class registration goes through `fastchart_arginfo.h`, generated
  from `fastchart.stub.php` by `~/php-src/build/gen_stub.php`. **Do
  not hand-edit `fastchart_arginfo.h`.** Run `/php-stub-regen` after
  any stub change.
- Memory: use PHP's `emalloc` / `efree` at the Zend boundary.
  `efree(NULL)` is undefined behavior in Zend — keep the `fc_efree_opt`
  null-guard wrapper rather than removing it as "dead code".
- Truecolor canvas only — `fastchart_require_truecolor()` rejects
  palette images at every `draw()` entry. New entry points must
  call it.
- Per-class state lives on the chart's typed C struct
  (`fastchart_<name>_obj`), not the generic `fastchart_obj`. Setters
  parse user input into typed C arrays at setter time; `draw()` is
  pure rasterisation.

### Release workflow

For maintainers cutting a new version: run `/release-ext` (or the
`/release` shim). It reads `.release-config`, runs the build matrix
(PHP-8.3, 8.4, 8.5), tags bare semver, drives the changelog
date-stamping. CHANGELOG follows Keep-a-Changelog; the
`PHP_FASTCHART_VERSION` literal in `php_fastchart.h` always holds
the next-to-tag version (no `-dev` suffix).

### License

By submitting a patch you agree to license your contribution under
the same license as the project (BSD 3-Clause).
