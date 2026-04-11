#!/bin/bash
# Build WiFiApp using Retro68 Docker image

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MAC_APP_DIR="$(dirname "$SCRIPT_DIR")"

echo "Building WiFiApp..."
echo "Source: $MAC_APP_DIR/WiFiApp"

docker run --rm --user $(id -u):$(id -g) -v "$MAC_APP_DIR":/root -i ghcr.io/autc04/retro68 /bin/bash <<"EOF"
    cd /root/WiFiApp
    rm -rf build
    mkdir build
    cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
    make
    echo ""
    echo "Build output:"
    ls -la
EOF

echo ""
echo "Output files:"
ls -la "$MAC_APP_DIR/WiFiApp/build/" 2>/dev/null || echo "Build directory not found"

# Set Finder flags in MacBinary file
FIRMWARE_DIR="$(dirname "$MAC_APP_DIR")"
if [ -f "$FIRMWARE_DIR/tools/set_macbinary_flags.py" ]; then
    echo ""
    echo "Setting Finder flags..."
    python3 "$FIRMWARE_DIR/tools/set_macbinary_flags.py" "$MAC_APP_DIR/WiFiApp/build/WiFi.bin" +bndl
fi
