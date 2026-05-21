#!/usr/bin/env bash
# Run perf.php against the currently-built modules/fastchart.so.
# Usage: bench/run.sh <label> [iters]
#   label   tag for this run (baseline, opt2, opt23, opt123)
#   iters   number of timed iterations per scenario (default 60)

set -euo pipefail

LABEL="${1:-unlabeled}"
ITERS="${2:-60}"

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PHP_BIN="${FC_BENCH_PHP:-$HOME/php-install-PHP-8.4-release/bin/php}"
EXT="$REPO_ROOT/modules/fastchart.so"

if [[ ! -x "$PHP_BIN" ]]; then
    echo "PHP not found at $PHP_BIN" >&2; exit 1
fi
if [[ ! -f "$EXT" ]]; then
    echo "extension not found at $EXT — run 'make' first" >&2; exit 1
fi

mkdir -p "$REPO_ROOT/bench/results"
OUTFILE="$REPO_ROOT/bench/results/$LABEL.json"

FC_BENCH_LABEL="$LABEL" \
FC_BENCH_ITERS="$ITERS" \
"$PHP_BIN" \
    -d extension="$EXT" \
    "$REPO_ROOT/bench/perf.php" \
    > "$OUTFILE"

echo "wrote $OUTFILE"
