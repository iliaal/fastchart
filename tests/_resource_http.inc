<?php
require __DIR__ . '/_resource_process.inc';

function fc_resource_http_start(): array
{
    $code = <<<'PHP'
$server = stream_socket_server('tcp://127.0.0.1:0', $errno, $error);
if ($server === false) exit(1);
echo stream_socket_get_name($server, false), "\n";
flush();
$requests = 0;
$deadline = microtime(true) + 15;
while (microtime(true) < $deadline) {
    $client = @stream_socket_accept($server, 1);
    if ($client === false) continue;
    stream_set_timeout($client, 2);
    $request = fgets($client);
    $stop = $request === "GET /stop HTTP/1.1\r\n";
    if (!$stop) $requests++;
    while (($line = fgets($client)) !== false && $line !== "\r\n") {}
    $body = $stop ? (string) $requests : base64_decode(
        'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=');
    $response = "HTTP/1.1 200 OK\r\nContent-Length: " . strlen($body)
        . "\r\nContent-Type: image/png\r\nConnection: close\r\n\r\n" . $body;
    while ($response !== '') {
        $written = @fwrite($client, $response);
        if (!$written) break;
        $response = substr($response, $written);
    }
    fclose($client);
    if ($stop) { fclose($server); exit(0); }
}
fclose($server);
exit(1);
PHP;
    $child = proc_open([PHP_BINARY, '-n', '-r', $code],
        [0 => ['pipe', 'r'], 1 => ['pipe', 'w'], 2 => ['pipe', 'w']], $pipes);
    if (!is_resource($child)) throw new RuntimeException('HTTP child unavailable');
    stream_set_timeout($pipes[1], 5);
    $address = trim((string) fgets($pipes[1]));
    if (!preg_match('/^127\.0\.0\.1:[0-9]+$/', $address)) {
        proc_terminate($child);
        throw new RuntimeException('HTTP listener unavailable');
    }
    return [$child, $pipes, 'http://' . $address];
}

function fc_resource_http_get(string $url): string
{
    $body = file_get_contents($url, false, stream_context_create([
        'http' => ['timeout' => 3, 'protocol_version' => 1.1],
    ]));
    if ($body === false) throw new RuntimeException('HTTP fixture request failed');
    return $body;
}
