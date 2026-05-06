--TEST--
Treemap: squarify lays cells, perturbing one item changes output, non-positive items are dropped
--EXTENSIONS--
fastchart
gd
--FILE--
<?php
/* Baseline: 5 cells, distinct values. */
$base_items = [
    ['label' => 'A', 'value' => 100],
    ['label' => 'B', 'value' => 70],
    ['label' => 'C', 'value' => 40],
    ['label' => 'D', 'value' => 30],
    ['label' => 'E', 'value' => 15],
];
$base = (new FastChart\Treemap(320, 200))->setItems($base_items)->renderPng();
echo "renders: ", ($base !== '' ? "yes" : "no"), "\n";

/* Perturbing the largest item visibly redistributes area, so output
 * must differ. */
$perturbed = $base_items;
$perturbed[0]['value'] = 200;  /* A doubles, others shrink proportionally */
$alt = (new FastChart\Treemap(320, 200))->setItems($perturbed)->renderPng();
echo "perturb_differs: ", ($alt !== $base ? "yes" : "no"), "\n";

/* Custom color flows through — different color, different bytes. */
$colored = $base_items;
$colored[0]['color'] = 0xFF0000;
$col = (new FastChart\Treemap(320, 200))->setItems($colored)->renderPng();
echo "color_differs: ", ($col !== $base ? "yes" : "no"), "\n";

/* Non-positive values are silently dropped. A treemap with one good
 * item and three rejected ones should match a treemap with just the
 * good item. */
$one_good = (new FastChart\Treemap(320, 200))
    ->setItems([['label' => 'solo', 'value' => 5]])
    ->renderPng();
$mixed = (new FastChart\Treemap(320, 200))
    ->setItems([
        ['label' => 'solo', 'value' => 5],
        ['label' => 'zero', 'value' => 0],
        ['label' => 'neg',  'value' => -3],
        ['label' => 'nan',  'value' => NAN],
    ])
    ->renderPng();
echo "drops_nonpositive: ", ($mixed === $one_good ? "yes" : "no"), "\n";

/* Empty / all-rejected: explicit error at draw time. */
try {
    (new FastChart\Treemap(320, 200))
        ->setItems([['value' => 0], ['value' => -1]])
        ->renderPng();
    echo "empty_throws: no throw (unexpected)\n";
} catch (\Error $e) {
    echo "empty_throws: throws\n";
}

/* Item cap: 256 items. The 257th gets dropped at setItems and the
 * render still succeeds. */
$big = [];
for ($i = 0; $i < 300; $i++) $big[] = ['value' => $i + 1];
$capped = (new FastChart\Treemap(640, 400))->setItems($big)->renderPng();
echo "cap_renders: ", ($capped !== '' ? "yes" : "no"), "\n";

/* setShowLabels(false) must change output (labels disappear). */
$labelled  = (new FastChart\Treemap(320, 200))
    ->setItems($base_items)->setShowLabels(true)->renderPng();
$unlabelled = (new FastChart\Treemap(320, 200))
    ->setItems($base_items)->setShowLabels(false)->renderPng();
echo "label_toggle: ", ($labelled !== $unlabelled ? "differs" : "same"), "\n";
?>
--EXPECT--
renders: yes
perturb_differs: yes
color_differs: yes
drops_nonpositive: yes
empty_throws: throws
cap_renders: yes
label_toggle: differs
