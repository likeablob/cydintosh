#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MAC_APP_DIR="$(dirname "$SCRIPT_DIR")"

echo "Building CydCtlApp..."
echo "Source: $MAC_APP_DIR/CydCtlApp"

docker run --rm --user $(id -u):$(id -g) -v "$MAC_APP_DIR":/root -i ghcr.io/autc04/retro68 /bin/bash <<'EOF'
    set -e
    cd /root/CydCtlApp
    rm -rf build
    mkdir build
    cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
    make
EOF

echo ""
echo "Build output:"
ls -la "$SCRIPT_DIR/build/"
echo ""
echo "Output files:"
ls -la "$SCRIPT_DIR/build/*.bin" 2>/dev/null || true

# Set Finder flags in MacBinary file
FIRMWARE_DIR="$(dirname "$MAC_APP_DIR")"
if [ -f "$FIRMWARE_DIR/tools/set_macbinary_flags.py" ]; then
    echo ""
    echo "Setting Finder flags..."
    python3 "$FIRMWARE_DIR/tools/set_macbinary_flags.py" "$SCRIPT_DIR/build/CydCtl.bin" +bndl
fi