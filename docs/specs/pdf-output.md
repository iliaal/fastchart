# Spec: `Chart::renderPdf()` via libharu

**Status:** deferred to v1.2
**Author:** ilia
**Created:** 2026-05-16
**Target:** v1.2

PDF output is the single biggest output-format gap against
ChartDirector (which ships PDF as a first-class format). fastchart
today emits SVG, PNG, JPEG, and WebP. Reporting / document-
generation users currently shell out to `rsvg-convert` or
`weasyprint` to get a PDF, which breaks the "single self-contained
extension" model and adds an external dependency to the deploy.

## Decisions locked

- **Backend: libharu (libHaru).** ZLib-licensed, alive (commits in
  2024+), stable C API since ~2008. Compatible with fastchart's
  BSD-3 license.
- **Optional at build time, mandatory in shipped binaries.** Source
  builds via PIE work without libharu — the `renderPdf()` method
  throws "PDF support not compiled in"; with `--with-libharu` it
  compiles in. Linux/macOS/Windows prebuilt binaries ship libharu
  statically linked, mirroring the 1.0.2 codec static-link model.
- **Architecture: third backend kind on `fastchart_target_t`.**
  Parallel to the existing SVG backend. Every chart family's
  `*_render_to_target` already calls through the
  `fastchart_target_line / rect / polygon / arc / ellipse / text /
  clip_push / clip_pop / image / gradient_*` primitives — once the
  PDF backend implements those, all 26 chart families + Symbol
  family render to PDF for free.
- **Text rendering: native PDF text** via `HPDF_Page_ShowText` is
  the default. Smaller files, searchable + selectable. A
  `setPdfTextMode(PDF_TEXT_PATHS)` toggle falls back to glyph-paths
  (same trick raster uses) for pixel-parity with raster output.
- **Distribution: static-linked, cached at build time.** Pinned
  `LIBHARU_VERSION` in `.github/workflows/release-linux.yml`; same
  pattern as the FT / PNG / JPEG / WEBP pins from 1.0.2. Cache key
  on the version pin. First build adds ~2 min; cache-hit unchanged.
  Binary grows ~6.8 MB → ~8 MB.

## Effort estimate

Sized at one focused week (~5-7 working days), two if Windows
libharu turns into a goose chase.

| Component | LoC | Time |
|-----------|-----|------|
| `config.m4` probe (`PKG_CHECK_MODULES([HARU], [libharu])`) | ~30 | 1h |
| `config.w32` probe (SDK deps server check) | ~20 | 2-3h iteration |
| `fastchart_target.h/c` — `FASTCHART_TARGET_PDF` kind + dispatch | ~300 | 4-6h |
| libharu primitive mappers (line, rect, polygon, arc, ellipse, text, clip-push/pop, image, gradient, shadow approximation) | ~500-700 | 1-2 days |
| Font loading: TTF path → `HPDF_LoadTTFontFromFile` → per-target cache | ~80 | 2-3h |
| Color: 0..255 int → 0..1 float helpers + alpha (HPDF `HPDF_GState`) | ~30 | 30min |
| Coordinate flip (PDF Y goes up) per page | inline | included |
| `renderPdf()` ZEND_METHOD + `renderToFile('*.pdf')` extension routing | ~80 | 2h |
| Stub + arginfo regen | ~10 | 15min |
| `fastchart_effects.c` PDF path: gradient via `HPDF_CreateAxialGradient`, drop shadow as offset-duplicate (same as SVG) | ~200 | 4-6h |
| phpt tests (PDF magic, page count, text-as-path baseline, image embed, every chart family smoke) | ~300 | 4-6h |
| `release-linux.yml` — build static libharu, mirror 1.0.2 codec static-link pattern | ~60 | 2-3h iteration |
| `windows.yml` — add libharu to SDK deps | ~10 | 1-2h iteration |
| Docs (CHANGELOG, README, `docs/examples/57_pdf_output.php`, gallery) | ~80 | 1-2h |
| **Total** | **~1500-2000** | **~5-7 days** |

## libharu primitive mapping

| `fastchart_target_*` | libharu equivalent |
|----------------------|--------------------|
| `_line(x0,y0,x1,y1,color,thick,dash)` | `HPDF_Page_MoveTo` + `LineTo` + `Stroke`; dash via `HPDF_Page_SetDash` |
| `_rect(x,y,w,h,color,fill,thick)` | `HPDF_Page_Rectangle` + `Fill` or `Stroke` or `FillStroke` |
| `_polygon(pts,n,color,fill,thick)` | `MoveTo` + `LineTo` loop + `ClosePathFillStroke` (or just `Fill` / `Stroke`) |
| `_arc(cx,cy,rx,ry,start,end,color,fill,thick)` | `HPDF_Page_Arc` (circular only); ellipse arc needs manual bezier decomposition — already have the math in `plutovg-path.c:add_arc` |
| `_ellipse(cx,cy,rx,ry,color,fill,thick)` | `HPDF_Page_Ellipse` |
| `_text(x,y,font,size,color,align,str)` | `HPDF_Page_BeginText` + `MoveTextPos` + `ShowText` + `EndText`; alignment via `HPDF_Page_TextWidth` measurement |
| `_clip_push / _clip_pop` | `HPDF_Page_GSave` + `Clip` + `HPDF_Page_GRestore` |
| `_image(x,y,w,h,bytes,mime)` | `HPDF_LoadPngImageFromMem` / `HPDF_LoadJpegImageFromMem` + `HPDF_Page_DrawImage` |
| `_gradient_rect / _gradient_polygon` | `HPDF_CreateAxialGradient` + `SetFill` with a soft-masked group; fall back to stripe-fill on radial |

## Hidden costs (deferred decisions)

- **Gradient fidelity**: libharu's `HPDF_CreateAxialGradient` works
  but stop count is limited and radial-via-cone is awkward. Decide
  in implementation: ship "good enough" axial gradients, document
  the radial trade-off, defer radial-perfect to future work.

- **Font embedding subset vs full**: `HPDF_LoadTTFontFromFile`
  with `embedding=HPDF_TRUE` embeds the full TTF (~few hundred KB
  per file). Subsetting requires walking the glyphs we actually
  drew — workable via FreeType but adds complexity. Default to
  full embed; subsetting is a v1.2.x polish.

- **Symbol family (Code128, QrCode)** uses the same target
  abstraction. PDF support comes free.

## Risks

- **Windows SDK availability**. php-windows-builder's SDK deps
  server may not have a current libharu. Fallback: vendor libharu
  sources into `vendor/libharu/` and build from source for Windows
  (the plutovg/plutosvg/qrcodegen precedent). Adds ~200 KB of
  vendored source code. Estimate +1 day if this fallback is needed.

- **libharu maintenance pace** is slow. Pin a specific version
  (currently 2.4.4 is latest stable); track upstream via the
  existing `.upstream/` mechanism so the release-ext freshness
  gate flags drift.

- **PDF text-vs-raster pixel parity is impossible** (different
  rasterizers, different hinting). What matters is "chart is
  recognizable + reads the same." Set this expectation in the
  feature CHANGELOG entry.

## Implementation order (de-risked)

1. **Backend skeleton + 5 primitives + `renderPdf()` stub** — line,
  rect, polygon, text, image. About a half-day. Validates the
  whole plumbing end-to-end and de-risks the rest.
2. **Remaining primitives** — arc, ellipse, clip, gradient. Another
  day.
3. **Per-chart-family smoke** — pick 3-4 representative families
  (Line, Bar, Pie, Stock) and verify PDF output is sane.
4. **Effects port** — gradient + drop shadow via `fastchart_effects.c`.
5. **Test suite** — phpt per representative family + format-check
  (magic header, page count, structural validity via `pdftotext`
  or `qpdf --check`).
6. **CI bundle** — static libharu in release-linux + windows
  workflows, cached.
7. **Docs** — example, gallery case, CHANGELOG entry.

## Out of scope (defer to v1.3+)

- **Multi-page PDFs**. v1.2 emits one chart per file. A
  `Chart::renderPdfPage(HpdfDocument)` API to stitch many charts
  into one PDF is a future addition; needs a wrapper around HPDF
  document state that crosses PHP API boundaries cleanly.
- **PDF/A or PDF/X compliance**. Standards-grade PDF for
  archival/print pipelines. Niche; libharu has partial support.
- **Embedded interactivity** (PDF annotations, links). PDF supports
  clickable areas analogous to image maps; would map naturally to
  the existing `setImageMap()` API but adds another integration
  point. Defer.
- **Custom font subsetting** (covered above).

## Alternatives considered, rejected

- **Cairo PDF surface**: bigger dependency, more complex build,
  pulls in fontconfig + pixman + pango on Linux. Skip.
- **Shell out to `rsvg-convert` / `weasyprint`**: breaks the
  self-contained extension model; adds an external dep at deploy
  time. Skip.
- **Pure-C custom PDF writer (no libharu)**: ~1000-1500 lines of
  PDF-spec implementation, no external dep. TCPDF does this in
  pure PHP but the C-side maintenance burden is real. Skip in
  favor of libharu unless libharu becomes unmaintained.

## Cross-references

- `vendor/plutosvg/source/plutosvg.c` — current SVG → vector path,
  parallel architecture lesson.
- `fastchart_target.h` / `fastchart_target.c` — backend abstraction.
- `fastchart_effects.c` — gradient + drop shadow implementation
  to port.
- `.github/workflows/release-linux.yml` — static-codec build
  pattern to mirror.
- `docs/specs/svg-to-raster.md` — prior spec doc shape.
