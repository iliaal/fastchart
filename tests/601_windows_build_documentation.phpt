--TEST--
Windows PDF build documentation distinguishes CI coverage from release artifacts
--FILE--
<?php
function check(bool $ok, string $label): void {
    echo $label, ': ', $ok ? 'ok' : 'FAIL', "\n";
}
$root = dirname(__DIR__);
$readme = str_replace("\r\n", "\n", file_get_contents($root . '/README.md'));
$config = file_get_contents($root . '/config.w32');
$workflow = str_replace("\r\n", "\n", file_get_contents($root . '/.github/workflows/windows.yml'));
$matched = preg_match('/CHECK_LIB\("(pdfio[^"\n]+)"/', $config, $libs);
check($matched === 1, 'library probe found');
$names = $matched ? explode(';', $libs[1]) : [];
$quoted = array_map(fn($name) => '`' . $name . '`', $names);
$last = array_pop($quoted);
$libraryText = implode(",\n", $quoted) . ', or ' . $last;
// Normalize whitespace, but assert the entire paragraph, not a suffix.
$normalize = fn($text) => preg_replace('/\s+/', ' ', trim($text));
preg_match('/On Windows, the PDF backend.*?(?=\n\n)/s', $readme, $paragraph);
$expected = "On Windows, the PDF backend is opt-in via `--with-pdfio` "
    . "(`config.w32`). Provision the pdfio headers and one of $libraryText first. "
    . 'A dedicated PHP 8.4 x64 NTS Windows CI lane builds with PDF enabled and requires every PDF test '
    . 'to pass without skipping. The ordinary Windows build matrix and published prebuilt DLLs remain '
    . 'PDF-disabled; the dedicated PDF lane does not publish release artifacts.';
check(isset($paragraph[0]) && $normalize($paragraph[0]) === $normalize($expected), 'complete Windows PDF claim');
check(str_contains($config, 'ARG_WITH("pdfio",') && str_contains($config, 'if (PHP_PDFIO != "no")')
    && str_contains($config, 'CHECK_HEADER_ADD_INCLUDE("pdfio.h"'), 'opt-in and headers');
preg_match('/^  windows-pdf:\n(.*?)(?=^  release:)/ms', $workflow, $pdf);
$pdf = $pdf[1] ?? '';
check(str_contains($pdf, "php-version: '8.4'") && str_contains($pdf, 'ts: nts')
    && str_contains($pdf, 'arch: x64') && str_contains($pdf, 'args: --enable-fastchart --enable-fastchart-dev --with-pdfio'), 'dedicated PDF configuration');
check(str_contains($pdf, "Get-ChildItem tests -Filter '*pdf*.phpt'")
    && str_contains($pdf, '$LASTEXITCODE -ne 0 -or $passed -lt $pdfTests.Count'), 'PDF tests must pass');
preg_match('/^  windows-build:\n(.*?)(?=^  windows-pdf:)/ms', $workflow, $matrix);
$matrix = $matrix[1] ?? '';
preg_match_all('/^\s+args: (.+)$/m', $matrix, $args);
check(count($args[1]) > 0 && !array_filter($args[1], fn($arg) => str_contains($arg, '--with-pdfio')), 'ordinary matrix PDF-disabled');
preg_match('/^  release:\n(.*)$/ms', $workflow, $release);
check(isset($release[1]) && preg_match('/^    needs: windows-build$/m', $release[1]) === 1
    && !str_contains($pdf, 'upload-artifact@') && !str_contains($pdf, 'php-windows-builder/release@'), 'PDF lane excluded from release');
?>
--EXPECT--
library probe found: ok
complete Windows PDF claim: ok
opt-in and headers: ok
dedicated PDF configuration: ok
PDF tests must pass: ok
ordinary matrix PDF-disabled: ok
PDF lane excluded from release: ok
