<?php
/* Funnel: descending stacked horizontal trapezoids. Common use:
 * conversion / drop-off views where the relative width of each
 * stage encodes the count, and the value labels on the right
 * carry the absolute number. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\Funnel(680, 420))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Signup → paid conversion')
    ->setStages([
        ['label' => 'Visit',   'value' => 12_400],
        ['label' => 'Sign up', 'value' =>  3_200],
        ['label' => 'Active',  'value' =>  1_950],
        ['label' => 'Trial',   'value' =>    820],
        ['label' => 'Paid',    'value' =>    310],
    ])
    ->renderToFile(__DIR__ . '/33_funnel.png');
