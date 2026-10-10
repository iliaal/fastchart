--TEST--
Funnel preserves complete formatted values across all styles
--EXTENSIONS--
fastchart
--FILE--
<?php
$funnel = (new FastChart\Funnel(600, 400))
    ->setSvgTextMode(FastChart\Chart::SVG_TEXT_NATIVE)
    ->setStages([['value' => 50], ['value' => 25]]);
foreach ([FastChart\Funnel::STYLE_FUNNEL, FastChart\Funnel::STYLE_PYRAMID,
          FastChart\Funnel::STYLE_CONE] as $style) {
    $funnel->setStyle($style);
    foreach (['%.300f', '%100.2f', 'value: %.2f%%'] as $format) {
        $funnel->setShowValues(true, $format);
        $svg = $funnel->renderSvg();
        $complete = true;
        foreach ([50, 25] as $value) {
            /* PHP's sprintf caps precision at 53; C's accepted format does not. */
            $label = $format === '%.300f' ? $value . '.' . str_repeat('0', 300)
                : sprintf($format, $value);
            $label = htmlspecialchars($label, ENT_QUOTES | ENT_XML1, 'UTF-8');
            $complete = $complete && strpos($svg, '>' . $label . '</text>') !== false;
        }
        echo $complete ? "complete\n" : "TRUNCATED\n";
        echo $svg === $funnel->renderSvg() ? "stable\n" : "UNSTABLE\n";
    }
    $funnel->setShowValues(false);
    echo strpos($funnel->renderSvg(), 'value:') === false ? "hidden\n" : "VISIBLE\n";
    $funnel->setShowValues(true, '');
    echo strpos($funnel->renderSvg(), '>50</text>') !== false ? "default\n" : "BAD DEFAULT\n";
}
?>
--EXPECT--
complete
stable
complete
stable
complete
stable
hidden
default
complete
stable
complete
stable
complete
stable
hidden
default
complete
stable
complete
stable
complete
stable
hidden
default
