<?php
/* v1.0 three-up gallery: SVG | PNG | JPG side-by-side for every chart
 * variant in the README. Drives the same case list as
 * scripts/build-readme-gallery.php; the only difference is the third
 * column (renderJpeg at the chart's default setJpegQuality=88).
 *
 * Output: docs/v1-gallery.html (self-contained; PNG + JPG via base64
 * data URI, SVG inlined). */

if (!class_exists('FastChart\\Chart')) {
    fwrite(STDERR, "fastchart.so not loaded\n");
    exit(1);
}

/* Reuse the case list from build-readme-gallery.php — read its
 * source and eval the prefix that builds $cases, stopping before
 * its own render loop. */
$src = file_get_contents(__DIR__ . '/build-readme-gallery.php');
$cut = strpos($src, '/* ----------- Build the HTML ----------- */');
if ($cut === false) { fwrite(STDERR, "case marker missing\n"); exit(1); }
$prefix = substr($src, 0, $cut);
$prefix = preg_replace('/^<\?php/', '', $prefix, 1);
$prefix = preg_replace('/^if \(!class_exists.*?\}/s', '', $prefix, 1);
eval($prefix);
if (!isset($cases) || !is_array($cases)) {
    fwrite(STDERR, "case array empty\n"); exit(1);
}

$rows = '';
$toc  = '';
$tot_svg = $tot_png = $tot_jpg = 0;

foreach ($cases as $idx => $case) {
    $c = $case['build']();
    $svg = $c->renderSvg();
    $png = $c->renderPng();
    $jpg = $c->renderJpeg();
    $tot_svg += strlen($svg);
    $tot_png += strlen($png);
    $tot_jpg += strlen($jpg);

    $sz_svg = number_format(strlen($svg) / 1024, 1);
    $sz_png = number_format(strlen($png) / 1024, 1);
    $sz_jpg = number_format(strlen($jpg) / 1024, 1);

    $png_uri = 'data:image/png;base64,'  . base64_encode($png);
    $jpg_uri = 'data:image/jpeg;base64,' . base64_encode($jpg);

    $label = htmlspecialchars($case['label'], ENT_QUOTES, 'UTF-8');
    $ref   = htmlspecialchars($case['ref'],   ENT_QUOTES, 'UTF-8');

    $n = $idx + 1;
    $toc .= "<li><a href=\"#row-{$n}\">{$label}</a></li>\n";

    $rows .= <<<HTML
<section class="row" id="row-{$n}">
  <h2>{$label}</h2>
  <p class="ref">Source: <a href="https://github.com/iliaal/fastchart/blob/master/{$ref}"><code>{$ref}</code></a></p>
  <div class="triple">
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
  </div>
</section>
HTML;
}

$ncharts = count($cases);
$kb_svg = number_format($tot_svg / 1024, 1);
$kb_png = number_format($tot_png / 1024, 1);
$kb_jpg = number_format($tot_jpg / 1024, 1);
$ver    = FastChart\Chart::version();
$now    = date('Y-m-d H:i');

$html = <<<HTML
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>fastchart {$ver}: SVG vs PNG vs JPG</title>
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

.triple { display: grid; grid-template-columns: repeat(3, 1fr); gap: 14px; }
@media (max-width: 1200px) { .triple { grid-template-columns: 1fr; } }
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
  <h1>fastchart {$ver} — SVG vs PNG vs JPG</h1>
  <p>{$ncharts} chart variants from the README, rendered three ways:
     <code>renderSvg()</code> (vector source),
     <code>renderPng()</code> (plutovg + libpng),
     <code>renderJpeg()</code> at default quality 88 (plutovg + libjpeg-turbo).
     Generated {$now}.</p>
  <p>Total bytes: SVG <code>{$kb_svg} KB</code> ·
                   PNG <code>{$kb_png} KB</code> ·
                   JPG <code>{$kb_jpg} KB</code>.</p>
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
