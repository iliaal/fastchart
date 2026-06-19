--TEST--
BarChart: lollipop stems and dumbbell connectors
--EXTENSIONS--
fastchart
simplexml
--FILE--
<?php

/* BAR_STYLE_LOLLIPOP draws one circle bullet per data point (stem +
 * tip); BAR_STYLE_DUMBBELL needs floating [min,max] data and draws two
 * bullets per point (one at each end). fastchart_target_ellipse emits
 * <circle> for equal radii, so the bullet count is countable. The plain
 * bar style draws no bullets. */

function valid(string $svg): bool {
    return strlen($svg) > 100 &&
        simplexml_load_string($svg, null, LIBXML_NOERROR | LIBXML_NOWARNING) !== false;
}

$data = [5, 8, 3, 9, 4];

$plain = (new FastChart\BarChart(520, 300))->setSeries($data)->renderSvg();
$lolli = (new FastChart\BarChart(520, 300))->setSeries($data)
    ->setBarStyle(FastChart\BarChart::BAR_STYLE_LOLLIPOP)->renderSvg();

echo "plain_valid: ", valid($plain) ? "yes" : "no", "\n";
echo "lollipop_valid: ", valid($lolli) ? "yes" : "no", "\n";
echo "plain_no_bullets: ", (substr_count($plain, '<circle') === 0 ? "yes" : "no"), "\n";
echo "lollipop_one_bullet_each: ",
    (substr_count($lolli, '<circle') === count($data) ? "yes" : "no"), "\n";

/* Dumbbell over floating data: two bullets per category. */
$pairs = [[2, 8], [3, 9], [1, 5]];
$dumb = (new FastChart\BarChart(520, 300))->setFloating(true)->setSeries($pairs)
    ->setBarStyle(FastChart\BarChart::BAR_STYLE_DUMBBELL)->renderSvg();

echo "dumbbell_valid: ", valid($dumb) ? "yes" : "no", "\n";
echo "dumbbell_two_bullets_each: ",
    (substr_count($dumb, '<circle') === 2 * count($pairs) ? "yes" : "no"), "\n";
echo "dumbbell_clean: ", (strpos($dumb, '-2147483648') === false ? "yes" : "no"), "\n";

/* Out-of-range style throws. */
try {
    (new FastChart\BarChart(520, 300))->setBarStyle(7);
    echo "bad_style: no-throw\n";
} catch (\ValueError $e) {
    echo "bad_style: threw\n";
}

echo "ok\n";
?>
--EXPECT--
plain_valid: yes
lollipop_valid: yes
plain_no_bullets: yes
lollipop_one_bullet_each: yes
dumbbell_valid: yes
dumbbell_two_bullets_each: yes
dumbbell_clean: yes
bad_style: threw
ok
