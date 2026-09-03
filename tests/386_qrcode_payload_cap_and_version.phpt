--TEST--
QrCode::setData() payload cap and Chart::version() module version
--EXTENSIONS--
fastchart
--INI--
asan.detect_leaks=0
--FILE--
<?php

echo "version matches: ",
    (FastChart\Chart::version() === phpversion('fastchart') ? 'yes' : 'no'), "\n";

$qr = new FastChart\QrCode();
try {
    $qr->setData(str_repeat('1', 7089));
    echo "qr max accepted: yes\n";
} catch (Throwable $e) {
    echo "qr max accepted: no\n";
}

try {
    (new FastChart\QrCode())->setData(str_repeat('1', 7090));
    echo "qr cap: accepted\n";
} catch (ValueError $e) {
    echo "qr cap: valueerror\n";
}

try {
    (new FastChart\Code128())->setData(str_repeat('A', 81));
    echo "code128 setData capped: accepted\n";
} catch (ValueError $e) {
    echo "code128 setData capped: ",
        str_contains($e->getMessage(), 'at most 80 characters') ? "valueerror\n" : "wrong\n";
}

?>
--EXPECT--
version matches: yes
qr max accepted: yes
qr cap: valueerror
code128 setData capped: valueerror
