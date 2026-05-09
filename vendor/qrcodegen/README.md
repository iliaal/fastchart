# Vendored: nayuki/QR-Code-generator (C variant)

- **Upstream**: https://github.com/nayuki/QR-Code-generator
- **Files vendored**: `qrcodegen.c`, `qrcodegen.h` (C variant only)
- **Vendored at SHA**: `2c9044de6b049ca25cb3cd1649ed7e27aa055138`
- **License**: MIT (see `LICENSE`; full text also retained verbatim in each
  source file's header)

## Why vendored

ext/fastchart calls `qrcodegen_encodeText` from `fastchart_qrcode.c` to
produce the dark-module grid for `FastChart\QrCode`. The encoder is small,
self-contained, and has no runtime dependencies, so vendoring is cheaper
than carrying a system-package probe (and avoids LGPL libqrencode).

## Do not modify

These files are tracked in-tree but treated as third-party. To update,
re-download from upstream HEAD and update the SHA above. Do not edit
in place — local patches will be lost on the next sync, and any encoder
bugs are upstream's to fix.
