# Vendored: nayuki/QR-Code-generator (C variant)

- **Upstream**: https://github.com/nayuki/QR-Code-generator
- **Files vendored**: `qrcodegen.c`, `qrcodegen.h` (C variant only)
- **Vendored at SHA**: `993ccbb6765327f5c49939ba77736098c04ab81d` (`c/` files
  byte-identical to the original import at
  `2c9044de6b049ca25cb3cd1649ed7e27aa055138`)
- **License**: MIT (see `LICENSE`; full text also retained verbatim in each
  source file's header)

## Why vendored

ext/fastchart calls `qrcodegen_encodeText` from `fastchart_qrcode.c` to
produce the dark-module grid for `FastChart\QrCode`. The encoder is small,
self-contained, and has no runtime dependencies, so vendoring is cheaper
than carrying a system-package probe (and avoids LGPL libqrencode).

## Local modifications

These files are tracked in-tree but treated as third-party. Keep local
changes to a minimum, mark each one with a `LOCAL-PATCH (fastchart):`
comment, and record it in `vendor/PATCHES.md`. To update, re-download from
upstream, update the SHA above, and re-apply every entry listed for
qrcodegen in `vendor/PATCHES.md`. Encoder bugs are upstream's to fix.
