--TEST--
Constructor with size args + renderToFile (auto-format from extension)
--EXTENSIONS--
fastchart
--FILE--
<?php

// Constructor: width + height set without setSize().
$bytes = (new FastChart\LineChart(640, 480))->setSeries([1, 5, 3, 8])->renderPng();
$signature = substr(bin2hex($bytes), 0, 8);
echo "ctor_png_sig: ", $signature === '89504e47' ? "ok" : "bad", "\n";

// PNG header carries width/height in big-endian at offset 16/20.
$w = (ord($bytes[16]) << 24) | (ord($bytes[17]) << 16) | (ord($bytes[18]) << 8) | ord($bytes[19]);
$h = (ord($bytes[20]) << 24) | (ord($bytes[21]) << 16) | (ord($bytes[22]) << 8) | ord($bytes[23]);
echo "ctor_dims: ", ($w === 640 && $h === 480 ? "640x480" : "$w x $h"), "\n";

// Constructor with no args keeps the 800x600 default.
$bytes = (new FastChart\LineChart)->setSeries([1, 2])->renderPng();
$w = (ord($bytes[16]) << 24) | (ord($bytes[17]) << 16) | (ord($bytes[18]) << 8) | ord($bytes[19]);
$h = (ord($bytes[20]) << 24) | (ord($bytes[21]) << 16) | (ord($bytes[22]) << 8) | ord($bytes[23]);
echo "default_dims: {$w}x{$h}\n";

// Mixed-null constructor args raise.
try {
    new FastChart\LineChart(800, null);
    echo "mixed_null: no throw\n";
} catch (\ValueError $e) {
    echo "mixed_null: ValueError ok\n";
}

try {
    new FastChart\LineChart(0, 100);
    echo "zero_size: no throw\n";
} catch (\ValueError $e) {
    echo "zero_size: ValueError ok\n";
}

// renderToFile picks format from extension.
$tmp = tempnam(sys_get_temp_dir(), 'fctest');
$tests = [
    'png'  => '89504e47',
    'jpg'  => 'ffd8',
    'jpeg' => 'ffd8',
    'webp' => '52494646',  // RIFF
];
foreach ($tests as $ext => $sig) {
    $path = "$tmp.$ext";
    $count = (new FastChart\LineChart(300, 200))->setSeries([1, 2, 3])->renderToFile($path);
    $bytes = file_get_contents($path);
    $head = bin2hex(substr($bytes, 0, strlen($sig) / 2));
    $ok = $head === $sig && $count > 0 && strlen($bytes) === $count;
    echo "$ext: ", ($ok ? "ok" : "bad ($head, $count, " . strlen($bytes) . ")"), "\n";
    unlink($path);
}
unlink($tmp);

// GIF is gone in v1.0.
try {
    (new FastChart\LineChart(100, 100))->setSeries([1])->renderToFile('/tmp/x.gif');
    echo "gif: no throw\n";
} catch (\Error $e) {
    echo "gif: dropped ok\n";
}

// Bad extension rejected.
try {
    (new FastChart\LineChart(100, 100))->setSeries([1])->renderToFile('/tmp/foo.bmp');
    echo "bad_ext: no throw\n";
} catch (\ValueError $e) {
    echo "bad_ext: ValueError ok\n";
}
?>
--EXPECT--
ctor_png_sig: ok
ctor_dims: 640x480
default_dims: 800x600
mixed_null: ValueError ok
zero_size: ValueError ok
png: ok
jpg: ok
jpeg: ok
webp: ok
gif: dropped ok
bad_ext: ValueError ok
