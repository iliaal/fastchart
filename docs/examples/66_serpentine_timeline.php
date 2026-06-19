<?php
/* SerpentineTimeline: ordered events laid out in rows that reverse
 * direction each line, so the connecting path snakes back and forth and
 * a long sequence fits a compact rectangle. Dates render above each
 * marker, labels below. Here, a product release roadmap. */

require __DIR__ . '/_bootstrap.php';

(new FastChart\SerpentineTimeline(720, 420))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Release roadmap')
    ->setEvents([
        ['label' => 'Kickoff',     'date' => 'Jan', 'color' => 0x6C8EBF],
        ['label' => 'Prototype',   'date' => 'Feb'],
        ['label' => 'Alpha',       'date' => 'Mar', 'color' => 0xD79B00],
        ['label' => 'Beta',        'date' => 'May'],
        ['label' => 'RC',          'date' => 'Jun', 'color' => 0x82B366],
        ['label' => 'GA',          'date' => 'Jul'],
        ['label' => 'v1.1',        'date' => 'Sep'],
        ['label' => 'v1.2',        'date' => 'Nov', 'color' => 0x9673A6],
    ])
    ->setColumns(4)
    ->renderToFile(__DIR__ . '/66_serpentine_timeline.png');
