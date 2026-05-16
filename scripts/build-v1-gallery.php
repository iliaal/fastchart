<?php
/* v1.0 four-up gallery: SVG | PNG | JPG | WebP side-by-side for every
 * chart variant in the README. Drives the same case list as
 * scripts/build-readme-gallery.php; columns cover renderSvg,
 * renderPng, renderJpeg (default setJpegQuality=88), and renderWebp.
 *
 * Output: docs/v1-gallery.html (self-contained; PNG + JPG + WebP via
 * base64 data URI, SVG inlined). */

if (!class_exists('FastChart\\Chart')) {
    fwrite(STDERR, "fastchart.so not loaded\n");
    exit(1);
}

$bundle = require __DIR__ . '/_gallery_cases.inc.php';
$cases  = $bundle['cases'];

$rows = '';
$toc  = '';
$tot_svg = $tot_png = $tot_jpg = $tot_webp = 0;

foreach ($cases as $idx => $case) {
    $c = $case['build']();
    $label = htmlspecialchars($case['label'], ENT_QUOTES, 'UTF-8');
    $ref   = htmlspecialchars($case['ref'],   ENT_QUOTES, 'UTF-8');
    $n = $idx + 1;
    $toc .= "<li><a href=\"#row-{$n}\">{$label}</a></li>\n";

    /* Image-map probe: chart families that populate area data after a
     * render expose it through getImageMap(). When non-empty, emit a
     * single working PNG + <map> instead of the four-format quad —
     * the point of the case is the clickable hot-spots, not codec
     * comparison. Mostly catches BarChart / PieChart / ScatterChart. */
    $png = $c->renderPng();
    $tot_png += strlen($png);
    $map_name = "chart-{$n}";
    $img_map = method_exists($c, 'getImageMap') ? $c->getImageMap($map_name) : '';

    if ($img_map !== '') {
        $sz_png = number_format(strlen($png) / 1024, 1);
        $png_uri = 'data:image/png;base64,' . base64_encode($png);
        $rows .= <<<HTML
<section class="row" id="row-{$n}">
  <h2>{$label}</h2>
  <p class="ref">Source: <a href="https://github.com/iliaal/fastchart/blob/master/{$ref}"><code>{$ref}</code></a></p>
  <p class="ref">Hover or click a bar — hot-spots come from <code>getImageMap()</code>.</p>
  <div class="single">
    <figure>
      <figcaption>PNG + HTML <code>&lt;map&gt;</code> <span class="size">{$sz_png} KB</span></figcaption>
      <div class="frame"><img src="{$png_uri}" usemap="#{$map_name}" alt="Chart with hot-spots">{$img_map}</div>
    </figure>
  </div>
</section>
HTML;
        continue;
    }

    $svg  = $c->renderSvg();
    $jpg  = $c->renderJpeg();
    $webp = $c->renderWebp();
    $tot_svg  += strlen($svg);
    $tot_jpg  += strlen($jpg);
    $tot_webp += strlen($webp);

    $sz_svg  = number_format(strlen($svg)  / 1024, 1);
    $sz_png  = number_format(strlen($png)  / 1024, 1);
    $sz_jpg  = number_format(strlen($jpg)  / 1024, 1);
    $sz_webp = number_format(strlen($webp) / 1024, 1);

    $png_uri  = 'data:image/png;base64,'  . base64_encode($png);
    $jpg_uri  = 'data:image/jpeg;base64,' . base64_encode($jpg);
    $webp_uri = 'data:image/webp;base64,' . base64_encode($webp);

    $rows .= <<<HTML
<section class="row" id="row-{$n}">
  <h2>{$label}</h2>
  <p class="ref">Source: <a href="https://github.com/iliaal/fastchart/blob/master/{$ref}"><code>{$ref}</code></a></p>
  <div class="quad">
    <figure>
      <figcaption>SVG (vector) <span class="size">{$sz_svg} KB</span></figcaption>
      <div class="frame">{$svg}</div>
    </figure>
    <figure>
      <figcaption>PNG (plutovg → libpng) <span class="size">{$sz_png} KB</span></figcaption>
      <div class="frame"><img src="{$png_uri}" alt="PNG render"></div>
    </figure>
    <figure>
      <figcaption>JPG q88 (plutovg → libjpeg-turbo) <span class="size">{$sz_jpg} KB</span></figcaption>
      <div class="frame"><img src="{$jpg_uri}" alt="JPG render"></div>
    </figure>
    <figure>
      <figcaption>WebP (plutovg → libwebp) <span class="size">{$sz_webp} KB</span></figcaption>
      <div class="frame"><img src="{$webp_uri}" alt="WebP render"></div>
    </figure>
  </div>
</section>
HTML;
}

$ncharts = count($cases);
$kb_svg  = number_format($tot_svg  / 1024, 1);
$kb_png  = number_format($tot_png  / 1024, 1);
$kb_jpg  = number_format($tot_jpg  / 1024, 1);
$kb_webp = number_format($tot_webp / 1024, 1);
$ver     = FastChart\Chart::version();
$now     = date('Y-m-d H:i');

$html = <<<HTML
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>fastchart {$ver}: SVG vs PNG vs JPG vs WebP</title>
<style>
:root {
  --bg: #fafafa; --fg: #1f2328; --muted: #656d76;
  --border: #d0d7de; --accent: #0969da; --code-bg: #f6f8fa;
}
html, body { margin: 0; padding: 0; background: var(--bg); color: var(--fg);
             font-family: -apple-system, BlinkMacSystemFont, "Segoe UI",
                          "Helvetica Neue", Arial, sans-serif;
             line-height: 1.5; }
header { background: #fff; border-bottom: 1px solid var(--border);
         padding: 20px 24px; }
header h1 { margin: 0 0 6px; font-size: 1.5rem; }
header p { margin: 0; color: var(--muted); font-size: 0.95rem; }
header code { background: var(--code-bg); padding: 1px 6px; border-radius: 4px;
              font-size: 0.85em;
              font-family: ui-monospace, "SF Mono", Menlo, monospace; }

nav.toc { background: #fff; border-bottom: 1px solid var(--border);
          padding: 14px 24px; }
nav.toc ol { margin: 0; padding-left: 22px;
             column-count: 2; column-gap: 24px;
             font-size: 0.9rem; }
nav.toc a { color: var(--accent); text-decoration: none; }
nav.toc a:hover { text-decoration: underline; }

main { padding: 24px; }
section.row { background: #fff; border: 1px solid var(--border);
              border-radius: 8px; padding: 18px; margin-bottom: 24px; }
section.row h2 { margin: 0 0 6px; font-size: 1.1rem; }
section.row .ref { margin: 0 0 14px; color: var(--muted); font-size: 0.85rem; }
section.row .ref code { background: var(--code-bg); padding: 1px 5px;
                        border-radius: 3px; font-size: 0.9em;
                        font-family: ui-monospace, "SF Mono", Menlo, monospace; }
section.row .ref a { color: var(--accent); text-decoration: none; }

.quad { display: grid; grid-template-columns: repeat(4, 1fr); gap: 14px; }
@media (max-width: 1400px) { .quad { grid-template-columns: repeat(2, 1fr); } }
@media (max-width:  720px) { .quad { grid-template-columns: 1fr; } }
.single { display: flex; justify-content: center; }
.single figure { max-width: 720px; }
figure { margin: 0; }
figcaption { font-size: 0.82rem; color: var(--muted); margin-bottom: 6px;
             display: flex; justify-content: space-between; align-items: baseline; }
.size { font-family: ui-monospace, "SF Mono", Menlo, monospace;
        font-size: 0.78rem; }
.frame { background: #fff; border: 1px solid var(--border); border-radius: 4px;
         padding: 4px; overflow: hidden; }
.frame img, .frame svg { display: block; max-width: 100%; height: auto; }
</style>
</head>
<body>
<header>
  <h1>fastchart {$ver} — SVG vs PNG vs JPG vs WebP</h1>
  <p>{$ncharts} chart variants from the README, rendered four ways:
     <code>renderSvg()</code> (vector source),
     <code>renderPng()</code> (plutovg + libpng),
     <code>renderJpeg()</code> at default quality 88 (plutovg + libjpeg-turbo),
     <code>renderWebp()</code> (plutovg + libwebp).
     Generated {$now}.</p>
  <p>Total bytes: SVG <code>{$kb_svg} KB</code> ·
                   PNG <code>{$kb_png} KB</code> ·
                   JPG <code>{$kb_jpg} KB</code> ·
                   WebP <code>{$kb_webp} KB</code>.</p>
</header>

<nav class="toc">
  <ol>
{$toc}
  </ol>
</nav>

<main>
{$rows}
</main>
</body>
</html>
HTML;

$out = __DIR__ . '/../docs/v1-gallery.html';
file_put_contents($out, $html);
fprintf(STDOUT, "wrote %s (%.1f KB across %d charts)\n",
    realpath($out), filesize($out) / 1024, $ncharts);
