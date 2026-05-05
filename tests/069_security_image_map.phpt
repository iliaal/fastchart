--TEST--
ScatterChart::getImageMap escapes HTML and rejects javascript:/data:/vbscript: URLs
--EXTENSIONS--
fastchart
--FILE--
<?php

// Active-content schemes get the entire <area> dropped, not sanitized.
$c = new FastChart\ScatterChart(400, 300);
$c->setPoints([
    [1.0, 2.0, 'href' => 'javascript:alert(1)', 'tooltip' => 'evil'],
    [2.0, 3.0, 'href' => 'data:text/html,<script>alert(1)</script>'],
    [3.0, 4.0, 'href' => 'vbscript:msgbox(1)'],
    [4.0, 5.0, 'href' => 'https://example.com/ok', 'tooltip' => 'safe'],
    [5.0, 6.0, 'href' => '/relative/page'],
    [6.0, 7.0, 'href' => '#anchor'],
    [7.0, 8.0, 'href' => 'mailto:a@b'],
])->renderPng();

$map = $c->getImageMap('test');

// Three dangerous schemes must be absent entirely.
echo "no_javascript: ",  (str_contains($map, 'javascript:') ? "FAIL" : "ok"), "\n";
echo "no_data:       ",  (str_contains($map, 'data:')       ? "FAIL" : "ok"), "\n";
echo "no_vbscript:   ",  (str_contains($map, 'vbscript:')   ? "FAIL" : "ok"), "\n";

// Four safe schemes must render.
echo "https_present: ",  (str_contains($map, 'https://example.com/ok') ? "ok" : "MISS"), "\n";
echo "rel_present:   ",  (str_contains($map, 'href="/relative/page"') ? "ok" : "MISS"), "\n";
echo "anchor_present:", (str_contains($map, 'href="#anchor"')         ? "ok" : "MISS"), "\n";
echo "mailto_present:", (str_contains($map, 'mailto:a@b')             ? "ok" : "MISS"), "\n";

// HTML metacharacters in a safe URL must be escaped, not raw.
$c2 = new FastChart\ScatterChart(400, 300);
$c2->setPoints([
    [1, 2, 'href' => '/page?a=1&b="x"<y>z'],
    [3, 4, 'href' => '/safe', 'tooltip' => '<script>alert("x")</script>'],
])->renderPng();
$map2 = $c2->getImageMap();

// Raw <script>, raw "&" outside entity form, raw "<" or ">" -- all must be absent.
echo "amp_escaped:   ",  (str_contains($map2, '&amp;')  ? "ok" : "MISS"), "\n";
echo "quot_escaped:  ",  (str_contains($map2, '&quot;') ? "ok" : "MISS"), "\n";
echo "lt_escaped:    ",  (str_contains($map2, '&lt;')   ? "ok" : "MISS"), "\n";
echo "gt_escaped:    ",  (str_contains($map2, '&gt;')   ? "ok" : "MISS"), "\n";
echo "no_raw_script: ",  (str_contains($map2, '<script>')   ? "FAIL" : "ok"), "\n";

// Map name must be alphanumeric / dash / underscore only -- attribute
// injection via crafted name is dropped to its safe characters.
$c3 = new FastChart\ScatterChart(400, 300);
$c3->setPoints([[1,2,'href'=>'/x']])->renderPng();
$map3 = $c3->getImageMap('"><script>alert(1)</script>');
echo "name_sanitized:", (str_contains($map3, '<script>') ? "FAIL" : "ok"), "\n";
echo "name_kept_alphanum: ", (str_contains($map3, 'name="scriptalert1script"') ? "ok" : "MISS"), "\n";
?>
--EXPECT--
no_javascript: ok
no_data:       ok
no_vbscript:   ok
https_present: ok
rel_present:   ok
anchor_present:ok
mailto_present:ok
amp_escaped:   ok
quot_escaped:  ok
lt_escaped:    ok
gt_escaped:    ok
no_raw_script: ok
name_sanitized:ok
name_kept_alphanum: ok
