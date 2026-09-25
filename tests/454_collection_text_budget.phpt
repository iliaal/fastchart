--TEST--
Collection text setters enforce a 64 KiB aggregate budget without replacing prior state
--EXTENSIONS--
fastchart
--FILE--
<?php

$full = str_repeat('A', 8192);
$exact = [$full, $full, $full, $full, $full, $full, $full, $full];
$over = [...$exact, 'x'];

echo 'pyramid exact: ', (new FastChart\PopulationPyramid(300, 200))
    ->setCategories($exact) instanceof FastChart\PopulationPyramid ? "yes\n" : "no\n";
echo 'violin exact: ', (new FastChart\ViolinPlot(300, 200))
    ->setGroups(array_map(
        fn($label) => ['label' => $label, 'values' => [1, 2]],
        $exact)) instanceof FastChart\ViolinPlot ? "yes\n" : "no\n";
$discarded = [
    ['label' => $full, 'values' => [1]],
    ...array_fill(0, 7, ['label' => $full, 'values' => []]),
    ['label' => 'x', 'values' => [NAN]],
];
echo 'violin discarded labels: ', (new FastChart\ViolinPlot(300, 200))
    ->setGroups($discarded) instanceof FastChart\ViolinPlot ? "yes\n" : "no\n";
echo 'wordcloud exact: ', (new FastChart\WordCloud(300, 200))
    ->setWords(array_map(
        fn($text) => ['text' => $text, 'weight' => 1],
        $exact)) instanceof FastChart\WordCloud ? "yes\n" : "no\n";
echo 'events exact: ', (new FastChart\SerpentineTimeline(300, 200))
    ->setEvents(array_fill(0, 4, ['label' => $full, 'date' => $full]))
    instanceof FastChart\SerpentineTimeline ? "yes\n" : "no\n";

$cases = [
    'pyramid' => [
        (new FastChart\PopulationPyramid(300, 200))
            ->setCategories(['kept'])
            ->setLeftSeries(['data' => [1]]),
        fn($chart) => $chart->setCategories($over),
    ],
    'violin' => [
        (new FastChart\ViolinPlot(300, 200))
            ->setGroups([['label' => 'kept', 'values' => [1, 2]]]),
        fn($chart) => $chart->setGroups([[
            'label' => $full, 'values' => [1, 2],
        ], ...array_map(
            fn($label) => ['label' => $label, 'values' => [1, 2]],
            [...array_slice($exact, 0, -1), 'x'])]),
    ],
    'wordcloud' => [
        (new FastChart\WordCloud(300, 200))
            ->setWords([['text' => 'kept', 'weight' => 1]]),
        fn($chart) => $chart->setWords(array_map(
            fn($text) => ['text' => $text, 'weight' => 1],
            $over)),
    ],
    'events' => [
        (new FastChart\SerpentineTimeline(300, 200))
            ->setEvents([['label' => 'kept', 'date' => 'today']]),
        fn($chart) => $chart->setEvents([[
            'label' => $full, 'date' => $full,
        ], ...array_map(
            fn($label) => ['label' => $label, 'date' => ''],
            array_slice($exact, 1))]),
    ],
];

foreach ($cases as $name => [$chart, $replace]) {
    $before = $chart->renderSvg();
    try {
        $replace($chart);
        echo "$name over: NOT REJECTED\n";
    } catch (ValueError) {
        echo "$name state: ", $chart->renderSvg() === $before
            ? "preserved\n" : "CHANGED\n";
    }
}

?>
--EXPECT--
pyramid exact: yes
violin exact: yes
violin discarded labels: yes
wordcloud exact: yes
events exact: yes
pyramid state: preserved
violin state: preserved
wordcloud state: preserved
events state: preserved
