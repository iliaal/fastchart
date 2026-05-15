<?php
/* SankeyChart: bipartite / multi-layer flow with bezier ribbons.
 * Node positions are computed from a topological pass; ribbon
 * widths are proportional to flow value. */

require __DIR__ . '/_bootstrap.php';

/* Four-layer e-commerce flow: Store -> Category -> Items -> Brands.
 * Modeled after the ChartExpo "Store Orders Analysis" example
 * (https://chartexpo.com/blog/sankey-chart-examples). One brand
 * (Samsung) receives from two items (Mobile + Tablet) — the rest
 * are 1:1, which exercises the multi-source-to-one-sink ribbon
 * stacking path. */
(new FastChart\SankeyChart(860, 540))
    ->setFontPath($font)
    ->setDpi($dpi)
    ->setTitle('Store orders: category -> item -> brand')
    ->setNodes([
        /* 0 */  ['label' => 'Online Store (621)', 'color' => 0x6C8EBF],
        /* 1 */  ['label' => 'Furnitures (162)',   'color' => 0xD6B656],
        /* 2 */  ['label' => 'Garments (191)',     'color' => 0x6FD6D6],
        /* 3 */  ['label' => 'Electronics (268)',  'color' => 0xC993D6],
        /* 4 */  ['label' => 'Desk (43)'],
        /* 5 */  ['label' => 'Sofa (73)'],
        /* 6 */  ['label' => 'Chair (46)'],
        /* 7 */  ['label' => 'Jeans (46)'],
        /* 8 */  ['label' => 'T-Shirt (104)'],
        /* 9 */  ['label' => 'Jackets (41)'],
        /* 10 */ ['label' => 'Mobile (39)'],
        /* 11 */ ['label' => 'Tablet (73)'],
        /* 12 */ ['label' => 'Laptop (156)'],
        /* 13 */ ['label' => 'Stickley (43)'],
        /* 14 */ ['label' => 'IKEA (73)'],
        /* 15 */ ['label' => 'Kartell (46)'],
        /* 16 */ ['label' => "Levi's (46)"],
        /* 17 */ ['label' => 'H&M (104)'],
        /* 18 */ ['label' => 'Puma (41)'],
        /* 19 */ ['label' => 'Samsung (112)'],
        /* 20 */ ['label' => 'Dell (156)'],
    ])
    ->setLinks([
        /* Store -> Category */
        ['from' => 0, 'to' => 1, 'value' => 162],
        ['from' => 0, 'to' => 2, 'value' => 191],
        ['from' => 0, 'to' => 3, 'value' => 268],
        /* Furnitures -> items */
        ['from' => 1, 'to' => 4, 'value' =>  43],
        ['from' => 1, 'to' => 5, 'value' =>  73],
        ['from' => 1, 'to' => 6, 'value' =>  46],
        /* Garments -> items */
        ['from' => 2, 'to' => 7, 'value' =>  46],
        ['from' => 2, 'to' => 8, 'value' => 104],
        ['from' => 2, 'to' => 9, 'value' =>  41],
        /* Electronics -> items */
        ['from' => 3, 'to' => 10, 'value' =>  39],
        ['from' => 3, 'to' => 11, 'value' =>  73],
        ['from' => 3, 'to' => 12, 'value' => 156],
        /* Items -> Brands (mostly 1:1; Samsung pulls from Mobile + Tablet) */
        ['from' =>  4, 'to' => 13, 'value' =>  43],
        ['from' =>  5, 'to' => 14, 'value' =>  73],
        ['from' =>  6, 'to' => 15, 'value' =>  46],
        ['from' =>  7, 'to' => 16, 'value' =>  46],
        ['from' =>  8, 'to' => 17, 'value' => 104],
        ['from' =>  9, 'to' => 18, 'value' =>  41],
        ['from' => 10, 'to' => 19, 'value' =>  39],
        ['from' => 11, 'to' => 19, 'value' =>  73],
        ['from' => 12, 'to' => 20, 'value' => 156],
    ])
    ->renderToFile(__DIR__ . '/47_sankey.png');
