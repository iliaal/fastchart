--TEST--
Funnel STYLE_CONE accepts proportional bands below the flat funnel height floor
--EXTENSIONS--
fastchart
--FILE--
<?php

/* A 300x80 cone has 45px of layout height after its bottom-arc
 * reserve. Sixteen equal bands are less than 4px high, but the
 * proportional layout still fits and shares every band boundary. */
$stages = array_fill(0, 16, ['value' => 1]);
$f = (new FastChart\Funnel(300, 80))->setStages($stages);
foreach ([FastChart\Funnel::STYLE_CONE, FastChart\Funnel::STYLE_PYRAMID] as $style) {
    $f->setStyle($style);
    try {
        $svg = $f->renderSvg();
        preg_match_all('/<polygon points="([^"]+)"/', $svg, $matches);
        $bands = [];
        $bounded = true;
        /* Each band is emitted twice, for its fill and outline. */
        foreach (array_chunk($matches[1], 2) as $pair) {
            $points = [];
            foreach (preg_split('/\s+/', trim($pair[0])) as $point) {
                [$x, $y] = array_map('floatval', explode(',', $point));
                $bounded = $bounded && $x >= 100 && $x <= 200 && $y >= 12 && $y <= 68;
                $points[] = [$x, $y];
            }
            $bands[] = $points;
        }
        echo $style === FastChart\Funnel::STYLE_CONE ? 'cone: ' : 'pyramid: ';
        echo count($bands) === 16 && $bounded ? "all bands fit\n" : "BAD geometry\n";
        if ($style === FastChart\Funnel::STYLE_CONE) {
            $joined = true;
            for ($i = 1; $i < count($bands); $i++) {
                $joined = $joined && array_slice($bands[$i], 0, 15)
                    === array_reverse(array_slice($bands[$i - 1], 15));
            }
            echo 'cone boundaries: ', $joined ? "shared\n" : "BAD\n";
            echo 'repeat render: ', $svg === $f->renderSvg() ? "stable\n" : "BAD\n";
        }
    } catch (ValueError $e) {
        echo "unexpected rejection: ", $e->getMessage(), "\n";
    }
}

$f->setStyle(FastChart\Funnel::STYLE_FUNNEL);
try {
    $f->renderSvg();
    echo "flat funnel: BAD accepted\n";
} catch (ValueError $e) {
    echo 'flat funnel: ', strpos($e->getMessage(), '4px') !== false ? "rejected\n" : "BAD error\n";
}
$f->setStyle(FastChart\Funnel::STYLE_CONE)->setSize(300, 35);
try {
    $f->renderSvg();
    echo "zero cone height: BAD accepted\n";
} catch (ValueError $e) {
    echo 'zero cone height: ', strpos($e->getMessage(), 'bottom-arc reserve') !== false ? "rejected\n" : "BAD error\n";
}
?>
--EXPECT--
cone: all bands fit
cone boundaries: shared
repeat render: stable
pyramid: all bands fit
flat funnel: rejected
zero cone height: rejected
