--TEST--
LineChart setStrict(true) rejects a non-numeric series gap with TypeError
--EXTENSIONS--
fastchart
--FILE--
<?php
/* Strict scalar gaps: a non-numeric cell under setStrict(true) throws
 * TypeError at setter time, before mutating (the prior series stays
 * renderable); null stays a legal gap; default mode coerces silently. */
$c = (new FastChart\LineChart(200, 100))
	->setSeries([1, 2, 3])
	->setStrict(true);
$before = $c->renderSvg();

try {
	$c->setSeries([1, 'xx', 3]);
	echo "gap: no throw\n";
} catch (TypeError $e) {
	echo 'gap: TypeError: ', $e->getMessage(), "\n";
}

echo 'prior state preserved: ',
	$c->renderSvg() === $before ? "yes\n" : "NO\n";

$c->setSeries([1, null, 3]);
echo 'null gap accepted: ',
	strlen($c->renderSvg()) > 0 ? "yes\n" : "NO\n";

$d = (new FastChart\LineChart(200, 100))->setSeries([1, 'xx', 3]);
echo 'default mode coerces: ',
	strlen($d->renderSvg()) > 0 ? "yes\n" : "NO\n";

?>
--EXPECT--
gap: TypeError: FastChart strict mode: series data must be numeric or null
prior state preserved: yes
null gap accepted: yes
default mode coerces: yes
