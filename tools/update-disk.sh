#!/bin/bash
set -e

cd "$(dirname "$0")/.."
FIRMWARE_DIR="$(pwd)"

# Default disk image path
DISK_IMG="${1:-$FIRMWARE_DIR/data/disk.img}"

if [ ! -f "$DISK_IMG" ]; then
    echo "Error: $DISK_IMG not found"
    echo "Usage: $0 [disk_image_path]"
    echo "Default: data/disk.img"
    exit 1
fi

for app in CydCtlApp WeatherApp WiFiApp; do
    echo "Building $app..."
    (cd "$FIRMWARE_DIR/mac-app/$app" && ./build.sh)
done

echo "Updating disk.img..."
hmount "$DISK_IMG"
hdel CydCtl 2>/dev/null || true
hdel Weather 2>/dev/null || true
hdel WiFi 2>/dev/null || true
hcopy -m "$FIRMWARE_DIR/mac-app/CydCtlApp/build/CydCtl.bin" :
hcopy -m "$FIRMWARE_DIR/mac-app/WeatherApp/build/Weather.bin" :
hcopy -m "$FIRMWARE_DIR/mac-app/WiFiApp/build/WiFi.bin" :
humount

echo "Done."