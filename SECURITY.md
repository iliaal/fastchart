# Security policy

fastchart is a PHP extension that renders charts and symbols to SVG,
PNG, JPEG, and WebP. The realistic threat surface is the data fed into
setters (series, OHLCV rows, slices, labels, paths, barcode payloads)
and the parsing/rasterization/encoding pipeline that turns those
inputs into native buffers.

## Supported versions

| Version | Supported          |
|---------|--------------------|
| 1.7.x   | :white_check_mark: |
| 1.6.x   | :white_check_mark: |

The two most recent minor versions receive security fixes.

## Reporting a vulnerability

**Do not file a public GitHub issue for security vulnerabilities.**

Use GitHub's private security advisory feature at
<https://github.com/iliaal/fastchart/security/advisories/new>
or email Ilia Alshanetsky <ilia@ilia.ws> directly.

Please include:

- Affected fastchart version (`php -r 'echo phpversion("fastchart");'`)
- PHP version (`php -v`)
- Relevant library versions if known: FreeType, libpng,
  libjpeg-turbo, libwebp
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
  annotations, barcode data, etc.) and any render entry point.
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
- SVG parsing and rasterization reachable through `renderPng()`,
  `renderJpeg()`, `renderWebp()`, `renderToFile()`, and the
  `svgTo*()` helpers.
- Vendored plutovg / plutosvg / qrcodegen issues that are reachable
  through fastchart's public API. We may coordinate fixes upstream,
  but reports are in scope here when fastchart ships the code.

Out of scope:

- Vulnerabilities in FreeType, libpng, libjpeg-turbo, or libwebp
  themselves: report directly to the respective upstream projects.
  fastchart picks up fixes through the system package manager.
- Resource exhaustion from intentionally rendering near the documented
  size/count caps. Bypasses of those caps or overflow paths before the
  cap checks are in scope.
- Behavior of the `setStrict(false)` default mode for chart types
  outside Line / Area / Bar / Funnel. Silent drop of malformed entries is
  documented (see AGENTS.md "Public API").

## Resource sizing for operators

The raster frame uses the Zend allocator and counts against PHP's
`memory_limit`, but vendor-owned native allocations do not. Size
worker memory budgets for both:

- The raster frame: `width x height` pixels x 4 bytes RGBA. It counts
  against PHP's `memory_limit` and `fastchart.max_render_pixels`
  (`PHP_INI_SYSTEM`, default 64M pixels); at the default cap the
  frame alone is ~256 MiB RSS.
- Decoded source images (`setBackgroundImage()` / `addIconAt()`):
  generated chart SVG reuses one image definition per distinct path,
  and plutosvg can retain a decoded surface on that shared image element.
  Retained pixel bytes are bounded by `fastchart.max_image_cache_bytes`
  (`PHP_INI_SYSTEM`, default 64M bytes). plutosvg loads these through
  plutovg's malloc-backed surfaces, outside PHP's `memory_limit`.
  Lowering the cache limit causes repeat decodes rather than rejecting
  a render; temporary decode allocations are not covered by this limit.

Rough render-memory allowance per concurrently active worker, using
positive configured caps (not an RSS upper bound):

```
max_render_pixels x 4 + max_image_cache_bytes + encoder workspace (~1 frame)
```

Add headroom for PHP/application state, SVG/parser data, transient image
decodes, allocator overhead, and codec-dependent workspace; the encoder
estimate above is not a fixed limit.

PHP-FPM uses child processes (`pm.max_children`); ZTS servers such as
FrankenPHP can run concurrent worker threads within one process. These
limits apply to individual renders/documents, not aggregate server
memory. Multiply the render allowance by peak concurrent workers when
sizing the server, accounting separately for process and shared memory.
Both INI settings are `PHP_INI_SYSTEM`: scripts cannot override the
operator's settings. See README "Raster memory" for the matching
runtime notes.
