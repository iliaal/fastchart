<?php
/* Funnel STYLE_CONE: pyramid layout with front-facing ellipse arcs
 * at each band's top and bottom edges. Visually suggests a 3D cone
 * seen from the side. Layout (apex at top, base at bottom, band
 * heights proportional to value) matches STYLE_PYRAMID exactly —
 * only the silhouette changes. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\Funnel(680, 420))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Signup → paid conversion (cone)')
    ->setStages([
        ['label' => 'Visit',   'value' => 12_400],
        ['label' => 'Sign up', 'value' =>  3_200],
        ['label' => 'Active',  'value' =>  1_950],
        ['label' => 'Trial',   'value' =>    820],
        ['label' => 'Paid',    'value' =>    310],
    ])
    ->setStyle(FastChart\Funnel::STYLE_CONE)
    ->renderToFile(__DIR__ . '/52_funnel_cone.png');
