# Security policy

fastchart is a PHP extension that renders charts onto caller-supplied
`\GdImage` canvases via libgd. The realistic threat surface is the
data fed into setters (series, OHLCV rows, slices, labels, paths) and
the parsing of user-controlled arguments before they reach libgd.

## Supported versions

| Version | Supported          |
|---------|--------------------|
| 0.1.x   | :white_check_mark: |

Once 1.0 ships, the two most recent minor versions will receive
security fixes.

## Reporting a vulnerability

**Do not file a public GitHub issue for security vulnerabilities.**

Use GitHub's private security advisory feature at
<https://github.com/iliaal/fastchart/security/advisories/new>
or email Ilia Alshanetsky <ilia@ilia.ws> directly.

Please include:

- Affected fastchart version (`php -r 'echo phpversion("fastchart");'`)
- libgd version (`php -r 'echo gd_info()["GD Version"];'`)
- A minimal reproducing case (PHP code + the data array or fixture
  that triggers it; small enough to inline in the report)
- Impact: crash / RCE / info disclosure / DoS / etc.
- Whether you've coordinated disclosure with anyone else

Acknowledgement within 7 days, fix or status update within 30. Once a
fix is released the advisory becomes public.

## Scope

In scope:

- Crashes, memory corruption, or read-after-free in fastchart's own
  code reachable from PHP. Any setter on the public API that accepts
  user input (`setSeries`, `setOhlcv`, `setSlices`, `setPoints`,
  `setBoxes`, `setBubbles`, `setTasks`, category / axis labels,
  annotations, etc.) and any draw entry point.
- Buffer overflows, integer overflows, or out-of-bounds reads in the
  array-to-typed-C parsers under `fastchart_*.c`.
- Arginfo / ZPP mismatches that cause undefined behavior reachable
  from PHP.
- Embedded-NUL handling in scalar setters and label arrays. The
  intended contract is "scalar setters reject, per-element labels
  drop silently"; deviations from that contract are bugs.
- File-path arguments to `setBackgroundImage()` and `renderToFile()`.
  These go through PHP's `php_check_open_basedir`; bypasses are in
  scope.
- Palette-canvas / non-truecolor `\GdImage` inputs. `draw()` rejects
  them with `ValueError`; a bypass that lets a palette canvas reach
  libgd's AA path is in scope.

Out of scope:

- Vulnerabilities in libgd, FreeType, libpng, libjpeg, libwebp, or
  libavif themselves: report directly to the respective upstream
  projects. fastchart picks up fixes by the system package manager
  bumping the shared library; the extension is dynamically linked
  against ext/gd's libgd.
- Vulnerabilities in ext/gd itself: report to PHP via
  <https://github.com/php/php-src/security/policy>.
- Resource exhaustion from rendering very large canvases or very
  long series. fastchart caps allocation sizes on user-array setters
  but does not bound canvas dimensions; the caller controls
  `imagecreatetruecolor()`.
- Behavior of the `setStrict(false)` default mode for chart types
  outside Line / Area / Bar. Silent drop of malformed entries is
  documented (see AGENTS.md "Strict-mode coverage gap").
