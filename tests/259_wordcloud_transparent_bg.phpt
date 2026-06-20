--TEST--
WordCloud: setTransparentBackground(true) suppresses the opaque canvas fill
--EXTENSIONS--
fastchart
--FILE--
<?php

/* fnd_b7ae2b83: WordCloud draws its own canvas (it bypasses draw_frame) and
 * always emitted an opaque full-canvas rect, ignoring transparent_bg. The fill
 * is now skipped when transparent output is requested. */

use FastChart\WordCloud;

$rect = '/<rect x="0" y="0" width="300" height="200"/';

$transparent = (new WordCloud())->setSize(300, 200)
    ->setTransparentBackground(true)
    ->setWords([['text' => 'alpha', 'weight' => 10], ['text' => 'beta', 'weight' => 5]])
    ->renderSvg();
echo "transparent_skips_canvas_rect: ", (preg_match($rect, $transparent) ? 'no' : 'yes'), "\n";

$opaque = (new WordCloud())->setSize(300, 200)
    ->setWords([['text' => 'alpha', 'weight' => 10]])
    ->renderSvg();
echo "opaque_keeps_canvas_rect: ", (preg_match($rect, $opaque) ? 'yes' : 'no'), "\n";

?>
--EXPECT--
transparent_skips_canvas_rect: yes
opaque_keeps_canvas_rect: yes
