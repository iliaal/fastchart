--TEST--
SVG conversion gates data images and use elements on the parsed tree, not on comment-like bytes in attributes or prologs
--EXTENSIONS--
fastchart
--FILE--
<?php
$png = 'iVBORw0KGgoAAAANSUhEUgAAAAQAAAAECAIAAAAmkwkpAAAAEElEQVR4nGP4z8AARwzEcQCukw/x0F8jngAAAABJRU5ErkJggg==';
$img = '<image href="data:image/png;base64,' . $png . '" width="16" height="16"/>';
$xlink = '<image xlink:href="data:image/jpeg;base64,' . $png . '" width="16" height="16"/>';
$use = '<defs><rect id="r" width="16" height="16" fill="red"/></defs><use href="#r"/>';
$h = '<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="16" height="16">';
$wrap = [
    'attr_comment' => fn($b) => "$h<rect id=\"a<!--\" width=\"1\" height=\"1\"/>$b<rect id=\"-->\" width=\"1\" height=\"1\"/></svg>",
    'attr_comment_text' => fn($b) => "$h<rect id=\"a<!--\" width=\"1\" height=\"1\"/>$b<text>--></text></svg>",
    'attr_cdata' => fn($b) => "$h<rect id=\"<![CDATA[\" width=\"1\" height=\"1\"/>$b<rect id=\"]]>\" width=\"1\" height=\"1\"/></svg>",
    'attr_pi' => fn($b) => "$h<rect id=\"<?\" width=\"1\" height=\"1\"/>$b<rect id=\"?>\" width=\"1\" height=\"1\"/></svg>",
    'attr_pi_single' => fn($b) => "$h<rect id='<?' width='1' height='1'/>$b<rect id='?>' width='1' height='1'/></svg>",
    'unknown_attr' => fn($b) => "$h<foo a=\"<!--\"/>$b<foo b=\"-->\"/></svg>",
    'doctype' => fn($b) => "<!DOCTYPE svg [<!--]>$h$b<rect id=\"-->\" width=\"1\" height=\"1\"/></svg>",
    'xmldecl' => fn($b) => "<?xml version=\"?>\" encoding=\"<!--\"?>$h$b<rect id=\"-->\" width=\"1\" height=\"1\"/></svg>",
];
$payloads = ['img' => [$img, 'data:image/'], 'xlink' => [$xlink, 'data:image/'], 'use' => [$use, '<use>']];
$bad = 0;
foreach (['svgToPng', 'svgToJpeg', 'svgToWebp'] as $m) {
    foreach ($wrap as $wn => $w) {
        foreach ($payloads as $pn => [$body, $needle]) {
            try {
                FastChart\Chart::$m($w($body));
                echo "$m $wn $pn: accepted\n";
                $bad++;
            } catch (ValueError $e) {
                if (!str_contains($e->getMessage(), "must not contain $needle")) {
                    echo "$m $wn $pn: wrong message: ", $e->getMessage(), "\n";
                    $bad++;
                }
            }
        }
    }
}
echo "bypass variants rejected: ", $bad === 0 ? "all" : "$bad failed", "\n";

/* Both kinds present: the data-image message wins, as before. */
try {
    FastChart\Chart::svgToPng($wrap['attr_comment']($img . $use));
} catch (ValueError $e) {
    echo str_contains($e->getMessage(), 'data:image/') ? "precedence kept\n" : "precedence changed\n";
}

/* Benign markup that resembles the bypass still renders unchanged. */
$shape = '<rect width="16" height="16" fill="#3366cc"/>';
$plain = FastChart\Chart::svgToPng("$h$shape</svg>");
$benign = [
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>$h$shape</svg>",
    "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">$h$shape</svg>",
    "$h<!-- <use href=\"#r\"/> data:image/png -->$shape</svg>",
    "$h<![CDATA[ <use/> data:image/png ]]>$shape</svg>",
    "$h<desc>a use element</desc>$shape</svg>",
    "$h<rect id=\"a&lt;!--\" width=\"0\" height=\"0\"/>$shape</svg>",
    "$h<image href=\"logo.png\" width=\"16\" height=\"16\"/>$shape</svg>",
];
foreach ($benign as $i => $svg) {
    echo "benign $i: ", FastChart\Chart::svgToPng($svg) === $plain ? "identical" : "changed", "\n";
}
?>
--EXPECT--
bypass variants rejected: all
precedence kept
benign 0: identical
benign 1: identical
benign 2: identical
benign 3: identical
benign 4: identical
benign 5: identical
benign 6: identical
