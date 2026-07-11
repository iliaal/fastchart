--TEST--
PDF: every concrete chart family emits a structurally valid vector PDF
--EXTENSIONS--
fastchart
--SKIPIF--
<?php
require __DIR__ . '/_font_candidates.inc';
if (fc_pick_font() === '') die("skip no system font available\n");
try {
    (new FastChart\LineChart(80, 60))->setSeries([1, 2, 3])->renderPdf();
} catch (\Error $e) {
    if (strpos($e->getMessage(), "PDF support not compiled in") !== false) {
        die("skip extension built without --with-pdfio\n");
    }
    throw $e;
}
?>
--INI--
asan.detect_leaks=0
--FILE--
<?php
/* Before this test, renderPdf() had coverage on only 4 chart families
 * (Line/Bar/Pie plus a rotated-label case in tests/200). A NaN/Inf
 * coordinate leaking into the content stream, or a family whose PDF
 * path never emitted a valid trailer, would go unseen. Build a minimal
 * instance of every concrete chart family and assert each renderPdf()
 * produces a %PDF- header, a startxref trailer keyword, and no literal
 * "nan"/"inf" tokens (a formatted non-finite coordinate).
 *
 * The drawing operands live inside pdfio's page content stream, which
 * is Flate-compressed by default, so a naive scan of the raw PDF bytes
 * can never see a non-finite coordinate. This test inflates each
 * FlateDecode stream and scans the decompressed operators. The test
 * PHP build is --disable-all (no ext/zlib), so the inflater is a
 * self-contained pure-PHP RFC1950/RFC1951 decoder rather than
 * gzuncompress(); it is cross-validated byte-for-byte against zlib in
 * the review notes. Binary streams (the ICC profile) are skipped via a
 * printable-ratio gate so their random bytes can't false-match. */

require __DIR__ . '/_font_candidates.inc';
$font = fc_pick_font();

/* Pull every stream object out of a PDF, resolving /Length (direct or
 * indirect `N 0 R`) so a spurious "endstream" byte inside compressed
 * data can't truncate the body. Returns [['flate'=>bool,'body'=>str]]. */
function fc_pdf_streams(string $pdf): array {
    $objlen = [];
    if (preg_match_all('/(\d+)\s+0\s+obj\s+(\d+)\s+endobj/', $pdf, $m, PREG_SET_ORDER)) {
        foreach ($m as $mm) {
            $objlen[(int) $mm[1]] = (int) $mm[2];
        }
    }
    $streams = [];
    $off = 0;
    while (($sp = strpos($pdf, 'stream', $off)) !== false) {
        if ($sp >= 3 && substr($pdf, $sp - 3, 3) === 'end') {
            $off = $sp + 6;
            continue;
        }
        $dstart = strrpos(substr($pdf, 0, $sp), '<<');
        $dict = $dstart === false ? '' : substr($pdf, $dstart, $sp - $dstart);
        $bodyStart = $sp + 6;
        if (substr($pdf, $bodyStart, 2) === "\r\n") {
            $bodyStart += 2;
        } elseif (($pdf[$bodyStart] ?? '') === "\n" || ($pdf[$bodyStart] ?? '') === "\r") {
            $bodyStart += 1;
        }
        $len = null;
        if (preg_match('/\/Length\s+(\d+)\s+0\s+R/', $dict, $lm)) {
            $len = $objlen[(int) $lm[1]] ?? null;
        } elseif (preg_match('/\/Length\s+(\d+)/', $dict, $lm)) {
            $len = (int) $lm[1];
        }
        if ($len === null) {
            $off = $sp + 6;
            continue;
        }
        $streams[] = [
            'flate' => strpos($dict, 'FlateDecode') !== false,
            'body'  => substr($pdf, $bodyStart, $len),
        ];
        $off = $bodyStart + $len;
    }
    return $streams;
}

/* Pure-PHP zlib/raw-DEFLATE inflater (RFC1950 + RFC1951). The test PHP
 * build is --disable-all, so ext/zlib's gzuncompress() is unavailable;
 * this stands in for it. Returns the inflated string, or null on error.
 * Ported from puff.c's canonical-Huffman decode. */
function fc_inflate(string $s): ?string {
    $n = strlen($s);
    if ($n < 2) {
        return null;
    }
    $pos = 0;
    $cmf = ord($s[0]);
    $flg = ord($s[1]);
    if (($cmf & 0x0f) === 8 && ((($cmf << 8) | $flg) % 31) === 0 && !($flg & 0x20)) {
        $pos = 2; /* skip 2-byte zlib header (no preset dict) */
    }
    $out = '';
    $bitbuf = 0;
    $bitcnt = 0;

    $getbit = function () use (&$bitbuf, &$bitcnt, &$pos, $s, $n): int {
        if ($bitcnt === 0) {
            if ($pos >= $n) {
                return -1;
            }
            $bitbuf = ord($s[$pos++]);
            $bitcnt = 8;
        }
        $b = $bitbuf & 1;
        $bitbuf >>= 1;
        $bitcnt--;
        return $b;
    };
    $getbits = function (int $need) use (&$getbit): int {
        $val = 0;
        for ($i = 0; $i < $need; $i++) {
            $bit = $getbit();
            if ($bit < 0) {
                return -1;
            }
            $val |= ($bit << $i);
        }
        return $val;
    };
    $construct = function (array $lengths, int $nsym): array {
        $count = array_fill(0, 16, 0);
        for ($i = 0; $i < $nsym; $i++) {
            $count[$lengths[$i]]++;
        }
        $count[0] = 0;
        $offs = array_fill(0, 16, 0);
        for ($len = 1; $len < 16; $len++) {
            $offs[$len] = $offs[$len - 1] + $count[$len - 1];
        }
        $symbol = array_fill(0, $nsym, 0);
        for ($i = 0; $i < $nsym; $i++) {
            if ($lengths[$i] !== 0) {
                $symbol[$offs[$lengths[$i]]++] = $i;
            }
        }
        return [$count, $symbol];
    };
    $decode = function (array $tbl) use (&$getbit): int {
        [$count, $symbol] = $tbl;
        $code = 0;
        $first = 0;
        $index = 0;
        for ($len = 1; $len <= 15; $len++) {
            $bit = $getbit();
            if ($bit < 0) {
                return -1;
            }
            $code |= $bit;
            $c = $count[$len];
            if ($code - $first < $c) {
                return $symbol[$index + ($code - $first)];
            }
            $index += $c;
            $first += $c;
            $first <<= 1;
            $code <<= 1;
        }
        return -1;
    };

    $lenbase = [3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258];
    $lenext  = [0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0];
    $distbase= [1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577];
    $distext = [0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13];
    $clorder = [16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15];

    while (true) {
        $bfinal = $getbit();
        if ($bfinal < 0) {
            return null;
        }
        $btype = $getbits(2);
        if ($btype < 0) {
            return null;
        }
        if ($btype === 0) {
            $bitbuf = 0;
            $bitcnt = 0;
            if ($pos + 4 > $n) {
                return null;
            }
            $len = ord($s[$pos]) | (ord($s[$pos + 1]) << 8);
            $pos += 4;
            if ($pos + $len > $n) {
                return null;
            }
            $out .= substr($s, $pos, $len);
            $pos += $len;
        } elseif ($btype === 1 || $btype === 2) {
            if ($btype === 1) {
                $lit = array_fill(0, 288, 0);
                for ($i = 0;   $i < 144; $i++) $lit[$i] = 8;
                for ($i = 144; $i < 256; $i++) $lit[$i] = 9;
                for ($i = 256; $i < 280; $i++) $lit[$i] = 7;
                for ($i = 280; $i < 288; $i++) $lit[$i] = 8;
                $littbl = $construct($lit, 288);
                $disttbl = $construct(array_fill(0, 30, 5), 30);
            } else {
                $hlit = $getbits(5) + 257;
                $hdist = $getbits(5) + 1;
                $hclen = $getbits(4) + 4;
                $cl = array_fill(0, 19, 0);
                for ($i = 0; $i < $hclen; $i++) {
                    $cl[$clorder[$i]] = $getbits(3);
                }
                $cltbl = $construct($cl, 19);
                $lengths = [];
                $total = $hlit + $hdist;
                while (count($lengths) < $total) {
                    $sym = $decode($cltbl);
                    if ($sym < 0) {
                        return null;
                    }
                    if ($sym < 16) {
                        $lengths[] = $sym;
                    } elseif ($sym === 16) {
                        $rep = $getbits(2) + 3;
                        $prev = $lengths[count($lengths) - 1];
                        for ($r = 0; $r < $rep; $r++) $lengths[] = $prev;
                    } elseif ($sym === 17) {
                        $rep = $getbits(3) + 3;
                        for ($r = 0; $r < $rep; $r++) $lengths[] = 0;
                    } else {
                        $rep = $getbits(7) + 11;
                        for ($r = 0; $r < $rep; $r++) $lengths[] = 0;
                    }
                }
                $littbl = $construct(array_slice($lengths, 0, $hlit), $hlit);
                $disttbl = $construct(array_slice($lengths, $hlit, $hdist), $hdist);
            }
            while (true) {
                $sym = $decode($littbl);
                if ($sym < 0) {
                    return null;
                }
                if ($sym === 256) {
                    break;
                }
                if ($sym < 256) {
                    $out .= chr($sym);
                } else {
                    $sym -= 257;
                    if ($sym >= 29) {
                        return null;
                    }
                    $length = $lenbase[$sym] + $getbits($lenext[$sym]);
                    $dsym = $decode($disttbl);
                    if ($dsym < 0 || $dsym >= 30) {
                        return null;
                    }
                    $distance = $distbase[$dsym] + $getbits($distext[$dsym]);
                    $olen = strlen($out);
                    if ($distance > $olen) {
                        return null;
                    }
                    for ($k = 0; $k < $length; $k++) {
                        $out .= $out[$olen - $distance + $k];
                    }
                }
            }
        } else {
            return null;
        }
        if ($bfinal) {
            break;
        }
    }
    return $out;
}

$ohlcv = [];
for ($i = 0; $i < 8; $i++) {
    $ohlcv[] = [1700000000 + $i * 86400,
                100 + $i, 102 + $i, 99 + $i, 101 + $i, 1000];
}

$families = [
    'LineChart'    => fn() => (new FastChart\LineChart(300, 200))
        ->setSeries([1, 2, 3, 4, 5]),
    'AreaChart'    => fn() => (new FastChart\AreaChart(300, 200))
        ->setSeries([1, 2, 3, 4, 5]),
    'BarChart'     => fn() => (new FastChart\BarChart(300, 200))
        ->setSeries([1, 5, 3]),
    'PieChart'     => fn() => (new FastChart\PieChart(300, 200))
        ->setSlices(['a' => 1, 'b' => 2, 'c' => 3]),
    'ScatterChart' => fn() => (new FastChart\ScatterChart(300, 200))
        ->setPoints([[1, 1], [2, 3], [3, 2]]),
    'StockChart'   => fn() => (new FastChart\StockChart(400, 250))
        ->setOhlcv($ohlcv),
    'RadarChart'   => fn() => (new FastChart\RadarChart(300, 300))
        ->setSeries([['data' => [3, 4, 5, 4, 3]]])
        ->setCategoryLabels(['a', 'b', 'c', 'd', 'e']),
    'BubbleChart'  => fn() => (new FastChart\BubbleChart(300, 200))
        ->setPoints([[1, 1, 10], [2, 3, 20], [3, 2, 15]]),
    'PolarChart'   => fn() => (new FastChart\PolarChart(300, 300))
        ->setSeries([['data' => [[0, 1], [45, 2], [90, 3]]]]),
    'SurfaceChart' => fn() => (new FastChart\SurfaceChart(300, 200))
        ->setGrid([[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
    'ContourChart' => fn() => (new FastChart\ContourChart(300, 200))
        ->setGrid([[1, 2, 3], [4, 5, 6], [7, 8, 9]]),
    'GaugeChart'   => fn() => (new FastChart\GaugeChart(300, 200))
        ->setValue(42),
    'GanttChart'   => fn() => (new FastChart\GanttChart(400, 200))
        ->setTasks([
            ['label' => 't1', 'start' => 0, 'end' => 5],
            ['label' => 't2', 'start' => 3, 'end' => 8],
        ]),
    'BoxPlot'      => fn() => (new FastChart\BoxPlot(300, 200))
        ->setBoxes([['min' => 1, 'q1' => 2, 'median' => 3, 'q3' => 4, 'max' => 5]]),
    'Treemap'      => fn() => (new FastChart\Treemap(300, 200))
        ->setItems([['label' => 'a', 'value' => 5], ['label' => 'b', 'value' => 3]]),
    'Funnel'       => fn() => (new FastChart\Funnel(300, 200))
        ->setStages([['label' => 'a', 'value' => 100], ['label' => 'b', 'value' => 50]]),
    'Waterfall'    => fn() => (new FastChart\Waterfall(300, 200))
        ->setBars([['label' => 'a', 'value' => 100], ['label' => 'b', 'value' => -20]]),
    'Heatmap'      => fn() => (new FastChart\Heatmap(300, 200))
        ->setGrid([[1, 2], [3, 4]]),
    'LinearMeter'  => fn() => (new FastChart\LinearMeter(300, 60))
        ->setValue(40),
    'BulletChart'  => fn() => (new FastChart\BulletChart(400, 80))
        ->setRange(0, 100)
        ->setBands([['from' => 0, 'to' => 60], ['from' => 60, 'to' => 85],
                    ['from' => 85, 'to' => 100]])
        ->setValue(72)->setTarget(80),
    'ParetoChart'  => fn() => (new FastChart\ParetoChart(400, 300))
        ->setBars([['label' => 'a', 'value' => 40],
                   ['label' => 'b', 'value' => 30],
                   ['label' => 'c', 'value' => 10]]),
    'CalendarHeatmap' => fn() => (new FastChart\CalendarHeatmap(600, 140))
        ->setData(['2026-01-05' => 3, '2026-02-14' => 9, '2026-03-15' => 5])
        ->setColorRamp(0xEEFFEE, 0x004400),
    'SunburstChart' => fn() => (new FastChart\SunburstChart(300, 300))
        ->setHierarchy(['label' => 'root', 'children' => [
            ['label' => 'A', 'value' => 10], ['label' => 'B', 'value' => 20]]]),
    'SankeyChart'  => fn() => (new FastChart\SankeyChart(400, 250))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 2, 'value' => 5],
                    ['from' => 1, 'to' => 2, 'value' => 3]]),
    'MarimekkoChart' => fn() => (new FastChart\MarimekkoChart(400, 300))
        ->setColumns([
            ['label' => 'Q1', 'segments' => [
                ['label' => 'x', 'value' => 30], ['label' => 'y', 'value' => 20]]],
            ['label' => 'Q2', 'segments' => [
                ['label' => 'x', 'value' => 40], ['label' => 'y', 'value' => 10]]],
        ]),
    'VectorChart'  => fn() => (new FastChart\VectorChart(300, 300))
        ->setVectors([['x' => 0, 'y' => 0, 'dx' => 1, 'dy' => 1],
                      ['x' => 1, 'y' => 0, 'dx' => -1, 'dy' => 1],
                      ['x' => 0, 'y' => 1, 'dx' => 1, 'dy' => -1]]),
    'ArcDiagram'   => fn() => (new FastChart\ArcDiagram(400, 200))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1],
                    ['from' => 1, 'to' => 2, 'value' => 2]]),
    'ChordDiagram' => fn() => (new FastChart\ChordDiagram(300, 300))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1],
                    ['from' => 1, 'to' => 2, 'value' => 2]]),
    'NetworkChart' => fn() => (new FastChart\NetworkChart(300, 300))
        ->setNodes([['label' => 'A'], ['label' => 'B'], ['label' => 'C']])
        ->setLinks([['from' => 0, 'to' => 1, 'value' => 1],
                    ['from' => 1, 'to' => 2, 'value' => 2]]),
    'PopulationPyramid' => fn() => (new FastChart\PopulationPyramid(300, 300))
        ->setCategories(['a', 'b', 'c'])
        ->setLeftSeries(['label' => 'L', 'data' => [1, 2, 3]])
        ->setRightSeries(['label' => 'R', 'data' => [2, 3, 1]]),
    'ViolinPlot'   => fn() => (new FastChart\ViolinPlot(300, 300))
        ->setGroups([['label' => 'X', 'values' => [1, 2, 3, 4, 3, 2]]]),
    'CirclePacking' => fn() => (new FastChart\CirclePacking(300, 300))
        ->setHierarchy(['children' => [['value' => 5], ['value' => 3], ['value' => 8]]]),
    'Pictogram'    => fn() => (new FastChart\Pictogram(300, 150))
        ->setTotal(10)->setValue(6),
    'VennDiagram'  => fn() => (new FastChart\VennDiagram(300, 300))
        ->setSets([['size' => 10], ['size' => 8]])
        ->setIntersections([['sets' => [0, 1], 'size' => 3]]),
    'WordCloud'    => fn() => (new FastChart\WordCloud(300, 300))
        ->setWords([['text' => 'alpha', 'weight' => 5],
                    ['text' => 'beta', 'weight' => 3],
                    ['text' => 'gamma', 'weight' => 8]]),
    'SerpentineTimeline' => fn() => (new FastChart\SerpentineTimeline(400, 200))
        ->setEvents([['label' => 'a', 'date' => 'Jan'],
                     ['label' => 'b', 'date' => 'Feb'],
                     ['label' => 'c', 'date' => 'Mar']]),
    'Dendrogram'   => fn() => (new FastChart\Dendrogram(300, 300))
        ->setHierarchy(['children' => [['value' => 5], ['value' => 3], ['value' => 8]]]),
    'Partition'    => fn() => (new FastChart\Partition(300, 300))
        ->setHierarchy(['children' => [['value' => 5], ['value' => 3], ['value' => 8]]]),
];

$fail = 0;
foreach ($families as $name => $build) {
    $c = $build();
    if (method_exists($c, 'setFontPath')) {
        $c->setFontPath($font);
    }
    $pdf = $c->renderPdf();

    $ok = true;
    if (!str_starts_with($pdf, "%PDF-")) {
        echo "FAIL $name: no %PDF- header\n"; $ok = false;
    }
    if (!str_contains($pdf, "startxref")) {
        echo "FAIL $name: no startxref trailer\n"; $ok = false;
    }
    if (strlen($pdf) < 400) {
        echo "FAIL $name: implausibly small (", strlen($pdf), " bytes)\n"; $ok = false;
    }
    /* A NaN/Inf coordinate surfaces as a literal "nan"/"inf" number
     * token. Scan two surfaces:
     *
     *   1. The uncompressed object/xref/trailer structure (strip the
     *      streams first — their bytes aren't PDF syntax). Word
     *      boundaries avoid matching the legitimate "/Info" trailer key.
     *   2. Every FlateDecode stream, inflated. This is where the drawing
     *      operands actually live; without inflating them the scan can
     *      never see a non-finite coordinate. The ICC-profile stream is
     *      binary, so gate on a printable-byte ratio to keep its random
     *      bytes from false-matching a word-boundary "nan"/"inf". */
    $scan = preg_replace('/stream.*?endstream/s', '', $pdf);
    if (preg_match('/\bnan\b/i', $scan)) {
        echo "FAIL $name: contains 'nan' token (structure)\n"; $ok = false;
    }
    if (preg_match('/\binf\b/i', $scan)) {
        echo "FAIL $name: contains 'inf' token (structure)\n"; $ok = false;
    }
    $scanned = 0;
    foreach (fc_pdf_streams($pdf) as $st) {
        if (!$st['flate']) {
            continue;
        }
        $inf = fc_inflate($st['body']);
        if ($inf === null || $inf === '') {
            continue;
        }
        $len = strlen($inf);
        $printable = strlen(preg_replace('/[^\x09\x0a\x0d\x20-\x7e]/', '', $inf));
        if ($printable / $len < 0.9) {
            continue; /* binary stream (ICC profile) — not drawing operands */
        }
        $scanned++;
        if (preg_match('/\bnan\b/i', $inf)) {
            echo "FAIL $name: contains 'nan' token (content stream)\n"; $ok = false;
        }
        if (preg_match('/\binf\b/i', $inf)) {
            echo "FAIL $name: contains 'inf' token (content stream)\n"; $ok = false;
        }
    }
    /* Every render must yield at least one inflatable, printable
     * drawing stream. $scanned == 0 means the extractor or inflater
     * regressed and the operand scan above ran on nothing — that must
     * fail loudly, not pass vacuously. */
    if ($scanned === 0) {
        echo "FAIL $name: no content stream was inflated and scanned\n";
        $ok = false;
    }
    if ($ok) echo "$name: ok\n";
    else $fail++;
}
echo $fail === 0 ? "ALL OK\n" : "FAILED $fail/" . count($families) . "\n";
?>
--EXPECT--
LineChart: ok
AreaChart: ok
BarChart: ok
PieChart: ok
ScatterChart: ok
StockChart: ok
RadarChart: ok
BubbleChart: ok
PolarChart: ok
SurfaceChart: ok
ContourChart: ok
GaugeChart: ok
GanttChart: ok
BoxPlot: ok
Treemap: ok
Funnel: ok
Waterfall: ok
Heatmap: ok
LinearMeter: ok
BulletChart: ok
ParetoChart: ok
CalendarHeatmap: ok
SunburstChart: ok
SankeyChart: ok
MarimekkoChart: ok
VectorChart: ok
ArcDiagram: ok
ChordDiagram: ok
NetworkChart: ok
PopulationPyramid: ok
ViolinPlot: ok
CirclePacking: ok
Pictogram: ok
VennDiagram: ok
WordCloud: ok
SerpentineTimeline: ok
Dendrogram: ok
Partition: ok
ALL OK
