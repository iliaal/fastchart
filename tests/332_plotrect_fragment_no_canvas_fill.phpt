--TEST--
setPlotRect + drawSvgFragment omits the full-canvas fill (compositing safe)
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php

/* In the compositing workflow (several charts stitched onto one host
 * canvas via setPlotRect + drawSvgFragment), a family that paints an
 * unconditional full-canvas rect wipes its neighbours. fastchart_paint_
 * canvas_bg() skips the canvas-wide fill when has_plot_rect is set, so
 * the fragment must not contain <rect x="0" y="0" width=W height=H>. */

$frag = (new FastChart\Treemap(400, 300))
    ->setItems([['label' => 'A', 'value' => 100], ['label' => 'B', 'value' => 70],
        ['label' => 'C', 'value' => 40]])
    ->setPlotRect(10, 10, 200, 150)
    ->drawSvgFragment();

$has_canvas = (bool)preg_match(
    '/<rect x="0" y="0" width="400" height="300"/', $frag);
echo 'full_canvas_rect: ', $has_canvas ? 'BAD (present)' : 'absent ok', "\n";

/* The fragment still has real content, confined to the plot rect. */
echo 'is_group: ', (strpos($frag, '<g') !== false ? 'yes' : 'no'), "\n";
echo 'has_content: ', (strpos($frag, '<rect x="10"') !== false ? 'yes' : 'no'), "\n";

echo "done\n";
?>
--EXPECT--
full_canvas_rect: absent ok
is_group: yes
has_content: yes
done
