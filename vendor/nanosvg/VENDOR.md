# NanoSVG vendored sources

Upstream: <https://github.com/memononen/nanosvg> (commit master at the time
of vendoring, 2026-05).

License: zlib (see `LICENSE`). Compatible with fastchart's BSD-3-Clause.

## Files

| File | Source | Modified? |
|---|---|---|
| `nanosvgrast.h` | upstream `src/nanosvgrast.h` | No |
| `nanosvg.h` | upstream `src/nanosvg.h` (header section only) | **Yes — trimmed** |
| `LICENSE` | upstream `LICENSE.txt` | No |

## Why nanosvg.h was trimmed

Upstream's `nanosvg.h` is one ~3000-line file that mixes two concerns:

1. **Public type declarations** (~120 lines): `NSVGimage`, `NSVGshape`,
   `NSVGpath`, `NSVGpaint`, `NSVGgradient`, plus the supporting enums.
2. **SVG parser implementation** (~2900 lines): an XML walker that turns
   SVG-string input into the public types.

fastchart constructs the public types programmatically from `gdPoint[]`
arrays — it never parses an SVG string — so it doesn't need any of the
parser code. The trim drops the `#ifdef NANOSVG_IMPLEMENTATION` block
and the parser function declarations (`nsvgParseFromFile`, `nsvgParse`,
`nsvgDuplicatePath`, `nsvgDelete`).

The rasterizer (`nanosvgrast.h`) is self-contained — it walks the
public types' linked lists and rasterizes; it never calls back into the
parser. The trim is safe for our use case.

## Maintenance

When updating NanoSVG:

1. Pull the new `nanosvgrast.h` verbatim.
2. Pull the new `nanosvg.h` and re-trim: keep everything before
   `#ifdef NANOSVG_IMPLEMENTATION`, keep the public type declarations,
   drop the parser function declarations
   (`nsvgParseFromFile`/`nsvgParse`/`nsvgDuplicatePath`/`nsvgDelete`),
   close the C++ extern guard, close the header guard. ~150 lines.
3. Verify no new types appear in `nanosvgrast.h` that aren't in our
   trimmed `nanosvg.h` (`grep -oE 'NSVG[A-Za-z]+' nanosvgrast.h`).
4. Re-run the test suite and visually verify the example gallery.

If upstream adds new public types that the rasterizer references,
include them in the trimmed header. Don't pull the parser
implementation back — it's dead weight for our use case.
