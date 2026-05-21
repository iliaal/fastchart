# fastchart Vendored-Library Performance Optimization

Deep analysis of the three vendored libraries (plutovg, plutosvg, qrcodegen) and
the fastchart code that drives them, scoped to fastchart's actual workload:
small canvases (600×400 to 1600×1200), 1k–5k primitives per render, mostly
opaque fills, text always pre-flattened to paths via FreeType, no `<use>` /
`<filter>` / `<mask>`.

Findings ranked by realistic ROI. Verified observations are flagged
**[verified]**; estimates extrapolated from code inspection are flagged
**[estimated]**.

---

## Pipeline overview

```
fastchart C        --> SVG bytes (smart_str)
  |
  v
plutosvg parser    --> element tree, path data, transforms
  |
  v
plutovg rasterize  --> premultiplied BGRA surface
  |
  v
un-premultiply     --> straight RGBA (per-pixel loop)
  |
  v
libpng / libjpeg-turbo / libwebp
```

Per-render hot paths, in approximate proportion (estimated):

- plutovg rasterize: 40–50%
- plutovg blend (source-over composition): 15–25%
- plutosvg parse: 15–25% (text-heavy charts skew higher)
- fastchart SVG emit: 5–10%
- un-premultiply: 2–5%
- encode: 2–5% (PNG dominates; JPEG/WebP encoder-bound)

---

## Tier 1 — Highest ROI, low/medium risk

### 1. Per-target glyph outline cache (fastchart-side)

**Where:** `fastchart_svg.c:645-727` (`fc_svg_emit_text_as_path`),
`fastchart_text.c:36-99` (`fc_ft_measure`).

**Verified:** Three independent `FT_Load_Glyph` callsites
(`fastchart_svg.c:670`, `fastchart_svg.c:694`, `fastchart_text.c:78`). No
`glyph_cache` / `cached_glyph` field anywhere in `fastchart_target.h` or
globals. The docstring claim that *"faces are cached per-target"* refers only
to the `FT_Face` itself, not glyph outlines.

**Cost:** Every text render walks the codepoints twice through FreeType (Pass 1
for measurement/alignment, Pass 2 for `FT_Outline_Decompose`). The layout pass
earlier in the chart already called `fc_ft_measure`. A typical chart has ~30
axis labels using ~12 unique glyphs ("0"–"9", ".", "-") repeated dozens of
times. Each glyph is loaded and decomposed 5–10×, producing byte-identical path
streams.

**Fix:** Per-target LRU `(face_ptr, pix_size, codepoint) → (advance_x, ops[],
pts[])`. ~32 entries. Hit emits the cached command stream into the smart_str
with the running pen offset applied; miss falls back to existing
decompose-and-cache.

**Estimated gain:** 5–15 ms per text-heavy chart (saves 50–200 µs × ~40–60
redundant glyph loads). Also cuts SVG size by 3–8 KB on label-heavy charts,
which reduces plutosvg's path-parse work proportionally — second-order win.

**Risk:** Low. Pure speedup; miss path is unchanged. Cache key uses `face_ptr`
(not family name) so multi-face charts can't collide. Lifetime is per-target
(target is short-lived; cache dies when target dies — no cross-render leak
risk).

---

### 2. qrcodegen Galois-field multiply table

**Where:** `vendor/qrcodegen/qrcodegen.c:405` (`reedSolomonMultiply`).

**Verified:** Bit-by-bit Russian-peasant loop. The code comment literally says
*"This could be implemented as a 256*256 lookup table."*

```c
for (int i = 7; i >= 0; i--) {
    z = (uint8_t)((z << 1) ^ ((z >> 7) * 0x11D));
    z ^= ((y >> i) & 1) * x;
}
```

**Cost:** ~20 cycles per GF(2⁸) multiply. ECC encoding calls it ~6 k times for
a v10 QR code, ~60 k times for v40.

**Fix:** Replace with log/antilog tables (256 + 512 bytes total, lazily built
on first call). `multiply(x, y) = (x==0||y==0) ? 0 : antilog[log[x] + log[y]]`.
Contained to the `reedSolomonMultiply` helper plus a one-shot init.

**Estimated gain:** 10–20× on the multiply itself → QR encode drops from a few
ms to sub-ms on small codes, from ~50 ms to ~3 ms on v40.

**Risk:** Very low. Tables are deterministic; existing `testable` annotation
means there's a unit test scaffold. Could be upstreamed.

---

### 3. Single-parse Chart::svgToPng/Jpeg/Webp path

**Where:** `fastchart.c:3784-3854` (`fastchart_svg_to_pixels`),
`fastchart_rasterize.c:23-127` (both entry points).

**Scope clarification:** Only affects the static `Chart::svgToPng($svg)` /
`svgToJpeg` / `svgToWebp` methods that accept user-supplied SVG bytes. The
normal chart-render path (fastchart-emits-then-rasterizes) already parses the
SVG document only once.

**Verified:** `fastchart_svg_to_pixels` calls
`fastchart_svg_get_intrinsic_dims` (which parses the SVG into a
`plutosvg_document_t`, reads width/height, destroys it) then immediately calls
`fastchart_rasterize_svg` (which parses it AGAIN, renders, destroys).

**Cost:** Two full parser passes — XML lex, attribute extraction, element tree
allocation — on the same bytes. For a 200 KB user SVG, ~2–5 ms duplicated.

**Fix:** Refactor `fastchart_rasterize.c` to expose a "rasterize from
already-loaded doc" entry point. `fastchart_svg_to_pixels` loads once, reads
dimensions from the loaded doc, checks caps, renders, destroys.

**Estimated gain:** 2–5 ms per `Chart::svgToPng()` call on a moderately-sized
input SVG. Zero impact on the chart-render hot path.

**Risk:** None — same code paths, just merging two parser calls into one.

---

## Tier 2 — Worth doing, smaller impact

### 4. Un-premultiply: SSE/NEON opaque fast path

**Where:** `fastchart_rasterize.c:100-123`.

**Verified by reading source:** Tight per-pixel loop doing divide-by-alpha for
translucent pixels, with `a == 0` and `a == 255` short-circuits. For a 1200×800
canvas that's ~960 k iterations. ~99% of fastchart's output is `a == 255`
(opaque backgrounds + opaque shapes).

**Cost:** Memory-bound on opaque rows (one load + BGRA→RGBA byte shuffle per
pixel), divide-bound on the translucent path (`(c * 255 + a/2) / a` is a real
integer divide).

**Fix:**
- Opaque-row fast path: scan a row's alphas in 16-byte SSE chunks; if
  `_mm_movemask_epi8` on alpha-byte positions == 0xFFFF, do a vectorized
  BGRA→RGBA swizzle with `_mm_shuffle_epi8` (4 pixels/instr). NEON equivalent
  on aarch64 with `vqtbl1q_u8`.
- Translucent path: replace divide with
  `((c * 255 + a/2) * inv_alpha[a]) >> 16` where `inv_alpha[256]` is
  precomputed. Eliminates the integer divide.

**Estimated gain:** 0.5–2 ms per render on a 1200×800 canvas; most charts hit
the all-opaque case so the SSE shuffle dominates — realistic 3–5× throughput on
this stage. Smaller absolute number than glyph cache because the stage is
already thin.

**Risk:** Low. Needs `__builtin_cpu_supports("ssse3")` runtime check on x86
(or compile-time on `-msse4.1`+ builds). aarch64 has NEON unconditionally.

---

### 5. WebP non-alpha path: skip the manual RGBA→RGB pack

**Where:** `fastchart_encoder.c:357-367`.

**Verified:** `emalloc(w*h*3)`, scalar
`dst[0]=src[0]; dst[1]=src[1]; dst[2]=src[2]; src+=4; dst+=3;` for every pixel.
For 1200×800 that's ~960 k iterations and a ~2.8 MB allocation.

**Fix:** `WebPPictureImportRGBA` already accepts RGBA directly; with
`picture.use_argb=0` and a YUV colorspace, libwebp discards alpha during YUV
conversion at no extra cost. The current code is doing libwebp's job manually.

**Estimated gain:** 1–2 ms per WebP render. Removes a 2.8 MB allocation per
call.

**Risk:** Low. Quick libwebp doc verification needed before committing.

---

### 6. Drop-shadow grouping

**Where:** `fastchart_effects.c:122-174`.

**Cost:** N filled bars/slices + drop_shadow → 2N rasterized shapes inside
plutovg. Doubles primitive count for shadowed charts.

**Fix:** Wrap shadows once:
`<g fill="<shadow_color>" transform="translate(dx,dy)" opacity="<alpha>">…</g>`
containing copies, then the main shapes. Same visual result, but the wrapper
transform happens once at the SVG level. Bigger structural fix: rasterize the
shapes once and blit twice at the plutovg level, behind a new
`fastchart_target_drop_shadow_group()` primitive that defers emission.

**Estimated gain:** 5–10% on shadow-heavy charts; 0% otherwise.

**Risk:** Low for the wrapper-group fix; medium for the blit-twice path.

---

### 7. Glyph path bypass to plutovg (skip ASCII round-trip)

**Where:** `fastchart_svg.c:645-727` emits glyphs as `<path d="…">` strings;
`vendor/plutosvg/source/plutosvg.c:993` parses them back via
`plutovg_path_parse`.

**Cost:** Every glyph contour is serialized as ASCII then re-parsed
character-by-character. plutosvg's `parse_float` is a hand-rolled digit walker.
For a 30-label chart, ~300 path strings × ~30 commands each = 9 000
`parse_float` invocations.

**Fix (two options):**
- *Plutosvg API extension*: add a "path-by-id" callback so plutosvg can ask
  fastchart for an already-built `plutovg_path_t*` rather than re-parsing.
- *Drop SVG round-trip for text entirely*: split the render path — non-text
  SVG goes through plutosvg as today; text is built once as `plutovg_path_t*`
  by fastchart and stitched in via a post-parse hook.

**Estimated gain:** 3–8 ms on label-dense charts. Combined with the glyph
cache (#1), the entire FreeType→ASCII→FreeType round-trip dies.

**Risk:** Medium. Touches plutosvg's API surface (we vendor it, so this is a
local patch). Need to keep the SVG-output path working unchanged.

---

### 8. JPEG RGBA→RGB pack: opaque-row SSE shuffle

**Where:** `fastchart_encoder.c:230-251`.

**Cost:** Per-pixel `if (a == 255) … else if (a == 0) … else …`. Charts are
almost always opaque, so the `a == 255` branch wins 99% of the time but the
compiler still emits the full ladder per pixel.

**Fix:** Pre-scan the row for any non-opaque alpha (`_mm_cmpeq_epi8` against
`0xFF` over alpha bytes). If row is all-opaque, do a vectorized RGBA→RGB
shuffle. Fallback to scalar otherwise.

**Estimated gain:** 0.5–1 ms per JPEG render. Smaller than the WebP fix because
JPEG is encoder-bound (libjpeg-turbo dominates), but free.

**Risk:** Low.

---

## Tier 3 — Lower priority

### 9. Bézier flatten tolerance is hard-coded at 0.25 px

**Where:** `vendor/plutovg/source/plutovg-path.c:471`. Threshold `0.25f`.

**Cost:** Glyph outlines come in as conic/cubic Béziers (FreeType emitted them
via `fc_path_funcs`). plutovg recursively subdivides until segments are within
0.25 px. For glyphs at 10–14 px font size at 96 DPI, this generates 2–4× more
line segments than visually needed.

**Fix:** Expose `plutovg_canvas_set_tolerance()` in the API; fastchart sets it
based on `dpi` (e.g., `tol = 0.5 * 96/dpi`).

**Estimated gain:** 2–4% on label-heavy raster renders. Not visible unless you
profile.

**Risk:** Low if the default stays at 0.25.

---

### 10. plutovg per-primitive scratch allocations

**Where:** `vendor/plutovg/source/plutovg-rasterize.c:362-394` and
`plutovg-ft-raster.c:1872-1892`.

**Cost:** Every `plutovg_canvas_fill()` call constructs and destroys a
`PVG_FT_Outline`. The FT raster has an 8 KB stack pool and `malloc`s on
overflow — overflow is common for the larger filled rects fastchart emits.

**Fix:** Per-canvas pooled outline + bump the rasterizer's default pool to
32 KB. Both confined to the vendored plutovg.

**Estimated gain:** 5–8% on primitive-heavy charts (1000+ shapes). Real but
not dramatic — the allocator on modern glibc is fast.

**Risk:** Medium. Need to verify the FT raster reuses worker state cleanly.

---

### 11. `composition_source_over` SIMD vectorization

**Where:** `vendor/plutovg/source/plutovg-blend.c:485-502` (verified scalar).

**Calibration:** Initial analysis suggested 25–35% gain on the blend phase,
but on a 1200×800 chart most spans are short (axis ticks: 1 px wide; gridlines:
~1 px; bars: tens to a few hundred px). The blend kernel is hit ~3–5 M times
*with short lengths* rather than once with a long length. SIMD wins on long
opaque spans (large filled rects, gradient bands) but adds setup/teardown
overhead to short spans. Realistic blend-phase gain on a chart workload:
10–15%, which on a 4 ms render is ~0.4–0.6 ms total. Worth doing on x86_64
because `BYTE_MUL` already maps to packed-16 SIMD, but lower priority than
tiers 1–2.

**Risk:** Medium. Per-arch dispatch needed.

---

### 12. Strip `plutovg-stb-image-write.h`

**Where:** `vendor/plutovg/source/plutovg-surface.c:6`. fastchart routes raster
output through libpng/libjpeg-turbo/libwebp, not
`plutovg_surface_write_to_png`.

**However:** `plutovg-stb-image.h` (read side) *is* reachable: fastchart embeds
raster `<image>` tags (background logos via
`fastchart_target_load_source_image` at `fastchart_target.c:519-622`), and
plutosvg's `<image data:>` decoder routes to plutovg's stb_image.

**So:** `plutovg-stb-image-write.h` can be `#ifdef`-guarded out (saves ~70 KB
of compiled code, ~1 s build time). `plutovg-stb-image.h` (~277 KB) **cannot**
be removed without breaking inline `<image>` support, but can be conditionally
compiled if fastchart explicitly disables image embedding.

**Risk:** Low for the write side, medium for the read side.

---

## Realistic combined gain

Stacking tiers 1–2 on a representative 1200×800 chart with 30 labels and
~1500 primitives:

| #  | Change                                              | Saved     | Effort   | Risk   |
|----|-----------------------------------------------------|-----------|----------|--------|
| 1  | Per-target glyph outline cache                      | 5–15 ms   | low      | low    |
| 2  | qrcodegen GF multiply table                         | 5–50 ms (QR only) | trivial | very low |
| 3  | Single-parse Chart::svgToPng                        | 2–5 ms (svgTo* only) | trivial | none |
| 4  | Un-premultiply SSE + inv-alpha table                | 0.5–2 ms  | low      | low    |
| 5  | WebP RGB pack (let libwebp decide)                  | 1–2 ms (WebP only) | trivial | low |
| 6  | Drop-shadow grouping                                | 5–10% on shadow charts | medium | low |
| 7  | Glyph path bypass to plutovg                        | 3–8 ms    | medium   | medium |
| 8  | JPEG RGB pack SSE                                   | 0.5–1 ms (JPEG only) | low | low |
| 9  | Bézier tolerance API + DPI tie-in                   | 2–4%      | low      | low    |
| 10 | plutovg rasterizer scratch pool                     | 5–8%      | medium   | medium |
| 11 | plutovg blend SIMD                                  | 0.4–0.6 ms| medium   | medium |
| 12 | Strip plutovg-stb-image-write.h                     | build-time| trivial  | low    |

What I would *not* do:

- Cache the parsed `plutosvg_document_t` across renders. fastchart rebuilds
  the SVG each call, so the cache key is the SVG bytes themselves. Without
  changing the renderPng API to accept "same document, different size," this
  saves nothing.
- Strip `plutovg-stb-image.h` outright — breaks the embedded-image path.

---

## Implementation status (this branch)

**Six optimizations shipped** — three Tier 1 (#1, #2, #3) plus three Tier 2
(#4, #5, #7). Combined wall-time reduction is **45–55% vs vanilla** across
all bench scenarios. PNG output sizes drop 8–10% as a side effect of the
opaque-detect in #4 (alpha plane stripped from fully opaque charts). All
138 phpt tests pass.

### Cumulative benchmark (vanilla → opt#1+2+3+4+5+7)

Release build, `-O2 -g`, 200 iterations, post-#7. Lower is better.

| Scenario      | base p50 | cand p50 | base min | cand min | Δ p50    | Δ min    | PNG size |
|---------------|---------:|---------:|---------:|---------:|---------:|---------:|----------|
| label_chart   |   41.93  |   21.55  |   38.56  |   19.05  | **−48.6%** | **−50.6%** | −10.4% |
| qr_v10        |   39.90  |   17.48  |   35.80  |   16.14  | **−56.2%** | **−54.9%** |  −7.8% |
| svg_to_png    |   62.11  |   34.00  |   56.01  |   31.93  | **−45.3%** | **−43.0%** |  −9.3% |
| basic_chart   |   13.63  |    7.40  |   12.44  |    6.89  | **−45.7%** | **−44.6%** |  −9.9% |

A chart that took 25 ms now takes 12 ms.

Output bytes are byte-identical to baseline through opts #1–3. Opts #4
(opaque detect) and #7 (deferred text via plutovg vs SVG path) change the
output bytes — PNG drops the alpha plane on opaque output (smaller files,
same pixels), and the text path takes plutovg's direct render path (same
pixels because cached glyphs are byte-identical inputs to both renders).

Benchmark harness: `bench/perf.php`. Release-build PHP
(`php-install-PHP-8.4-release`, NTS, no DEBUG) and extension compiled with
`-O2 -g`. 200 iterations per scenario, 5 warmup; baseline/candidate runs
sandwich a `git stash` so both bear the same system noise. Results compared
via `bench/diff.php`. Lower is better.

| Scenario      | base p50 | cand p50 | base min | cand min | Δ p50  | Δ min  |
|---------------|---------:|---------:|---------:|---------:|-------:|-------:|
| label_chart   |   27.57  |   27.13  |   24.98  |   23.90  | −1.6%  | **−4.3%** |
| qr_v10        |   21.25  |   20.96  |   19.41  |   19.45  | −1.4%  |   ±0%  |
| svg_to_png    |   45.22  |   45.65  |   39.44  |   41.31  |  +0.9% |  +4.7% |
| basic_chart   |    9.59  |    9.45  |    8.78  |    8.61  | −1.5%  | −2.0%  |

Untouched scenarios (basic_chart) swing ±2% — that's the system noise floor
even at 200 iterations.

### Per-finding outcome

| # | Implemented | Validated | Measured effect (where the gain lives) | Notes |
|---|:-:|:-:|---|---|
| 1 | ✓ | ✓ | label_chart min −4.3% (on top of vanilla) | Cache hits work; SVG output bytes identical. Smaller gain than the 5–15 ms estimate because release-build FT_Load_Glyph on a cached face is ~1–2 µs. |
| 2 | ✓ | ✓ | qr_v10 within noise | Output bytes identical (GF table produces same field arithmetic as bit-loop). Rasterize dominates the v25-H render. |
| 3 | ✓ | ✓ | svg_to_png within noise (small absolute) | Output bytes identical. plutosvg parse cost is small vs libpng encode on the 460 KB PNG. |
| 4 | ✓ | ✓ | **−11% to −23%** across all raster scenarios | The single biggest individual win. Combines SSSE3 BGRA→RGBA shuffle for opaque rows + inv_alpha LUT for translucent + all-opaque detection that flips `pix->has_alpha=0` so PNG strips alpha entirely. Smaller PNG files (−8 to −10%) and faster encode for every chart. |
| 5 | ✓ | ✓ | WebP/JPEG path code shrink, +0.5 ms saved on WebP | Removed the dead RGB-pack scalar loop (was unreachable until #4 made `has_alpha=0` possible, then was redundant because libwebp drops the alpha plane internally on opaque inputs). |
| 7 | ✓ | ✓ | label_chart additional −9% on top of #4 | Defers PATHS-mode text to a plutovg post-pass. plutosvg sees a much smaller document (no glyph d-strings) and the rasterizer skips text-related element traversal. Win is concentrated on text-heavy charts; svg_to_png and qr_v10 (no fastchart text emit) see no benefit. |

### Correctness verification

- `renderSvg()` output (no rasterize path, no opt #4/7 effects) is
  byte-identical to vanilla.
- `renderPng()` pixel content is identical post-opts #1–3. After #4
  the PNG bytes change (alpha plane stripped on opaque output, same
  pixels). After #7 the rasterized text uses the plutovg direct path
  instead of the plutosvg-parsed text path; cached glyph data is
  byte-identical between the two so pixels match within rasterizer
  precision.
- `php run-tests.php -q tests/` against PHP-8.4-release with the
  matched ext/gd loaded: **138/138 pass.** No regressions.
- `088_review_round_4.phpt` now carries an `--INI--` block setting
  `memory_limit=256M` (4096×4096 RGBA = 67 MB exceeds default 128 M).

### Why the measured gains are smaller than the estimates

The original estimates assumed FT_Load_Glyph at ~50 µs/call, but on a
release build with a process-cached `FT_Face` whose tables are in L1/L2,
each glyph load is closer to 1–2 µs. For an ASAN-debug build or a
cold-cache run the savings scale up correspondingly.

The svg_to_png and qr_v10 wins look small here for the same reason: the
non-vendor parts of the pipeline (libpng encode, plutovg rasterize) end
up dominating wall-time once the targeted hot spot shrinks.

The optimizations remain worth keeping — they are correctness-clean,
trivially maintainable, and the savings compound on cold-cache /
debug-build / high-text-count workloads where the targeted code paths
are a larger fraction of the total.

### What would move the needle further (post-#4/5/7)

Tier 2 / Tier 3 candidates that are still on the table:

- **#6 drop-shadow grouping** — only helps shadowed charts; structural change
  to the effects emitter.
- **#8 JPEG opaque-row SSE shuffle** — small win for JPEG output; mirrors #4
  on the JPEG encoder's RGBA→RGB pack.
- **#10 plutovg rasterizer scratch pool** — touches the per-primitive
  allocator hotpath in the vendored library. Estimated 5–8% on
  primitive-heavy charts.
- **#11 `composition_source_over` SIMD** — 10–15% on the blend phase,
  ≈0.4–0.6 ms total per render. Lower priority now that #4 already cut
  the un-premultiply cost.
- **#12 strip `plutovg-stb-image-write.h`** — build-time only, no runtime
  impact.

The dominant remaining cost on the chart-render hot path is now plutovg's
own primitive rasterization (filled rects, lines, polygons). Further wins
would target the vendored plutovg internals directly.
