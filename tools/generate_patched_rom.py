#!/usr/bin/env python3
"""
Generate patched ROM for umac
Patches Mac Plus ROM for 240x320 resolution and more
Based on the umac source code
"""

import sys
import struct

# Configuration - 240x320 portrait mode
DISP_WIDTH = 240
DISP_HEIGHT = 320
ROM_SIZE = 0x20000  # 128KB
ROM_PLUSv3_VERSION = 0x4D1F8172
ROM_PLUSv3_SONYDRV = 0x17D30
M68K_INST_NOP = 0x4E71

# Sony driver from sonydrv.h
SONY_DRIVER = bytes(
    [
        0x6F,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x18,
        0x00,
        0x4C,
        0x00,
        0x56,
        0x00,
        0x68,
        0x00,
        0x94,
        0x05,
        0x2E,
        0x53,
        0x6F,
        0x6E,
        0x79,
        0x48,
        0xE7,
        0x00,
        0xC0,
        0x10,
        0x3C,
        0x00,
        0x1E,
        0xA7,
        0x1E,
        0x24,
        0x48,
        0x4C,
        0xDF,
        0x03,
        0x00,
        0xB5,
        0xFC,
        0x00,
        0x00,
        0x00,
        0x00,
        0x67,
        0x00,
        0x00,
        0x18,
        0x26,
        0x7A,
        0x00,
        0x64,
        0x16,
        0xBC,
        0x00,
        0x00,
        0x20,
        0x3C,
        0x00,
        0x01,
        0xFF,
        0xFB,
        0x20,
        0x4A,
        0x5C,
        0x88,
        0xA0,
        0x4E,
        0x4E,
        0x75,
        0x70,
        0xE9,
        0x4E,
        0x75,
        0x24,
        0x7A,
        0x00,
        0x4A,
        0x14,
        0xBC,
        0x00,
        0x01,
        0x60,
        0x1A,
        0x24,
        0x7A,
        0x00,
        0x40,
        0x14,
        0xBC,
        0x00,
        0x02,
        0x0C,
        0x68,
        0x00,
        0x01,
        0x00,
        0x1A,
        0x66,
        0x0A,
        0x4E,
        0x75,
        0x24,
        0x7A,
        0x00,
        0x2E,
        0x14,
        0xBC,
        0x00,
        0x03,
        0x32,
        0x28,
        0x00,
        0x06,
        0x08,
        0x01,
        0x00,
        0x09,
        0x67,
        0x0C,
        0x4A,
        0x40,
        0x6F,
        0x02,
        0x42,
        0x40,
        0x31,
        0x40,
        0x00,
        0x10,
        0x4E,
        0x75,
        0x4A,
        0x40,
        0x6F,
        0x04,
        0x42,
        0x40,
        0x4E,
        0x75,
        0x2F,
        0x38,
        0x08,
        0xFC,
        0x4E,
        0x75,
        0x70,
        0xE8,
        0x4E,
        0x75,
        0x00,
        0x00,
        0x00,
        0x00,
    ]
)


def rom_write16(rom, offset, value):
    """Write 16-bit value to ROM (big endian)"""
    rom[offset] = (value >> 8) & 0xFF
    rom[offset + 1] = value & 0xFF


def rom_write32(rom, offset, value):
    """Write 32-bit value to ROM (big endian)"""
    rom[offset] = (value >> 24) & 0xFF
    rom[offset + 1] = (value >> 16) & 0xFF
    rom[offset + 2] = (value >> 8) & 0xFF
    rom[offset + 3] = value & 0xFF


def screen_coord(x, y, screen_base, bytes_per_row):
    """Calculate screen coordinate for 24-bit Mac address space"""
    return screen_base + (bytes_per_row * y) + (x // 8)


def patch_rom(rom_data, force=False):
    """Apply all patches for 240x320 resolution"""
    rom = bytearray(rom_data)

    # Verify ROM version
    version = struct.unpack(">I", rom[0:4])[0]
    if version != ROM_PLUSv3_VERSION:
        if force:
            print(
                f"Warning: ROM version {version:08x} != expected {ROM_PLUSv3_VERSION:08x}"
            )
            print("Force flag set, continuing anyway...")
        else:
            print(
                f"Error: ROM version {version:08x} != expected {ROM_PLUSv3_VERSION:08x}"
            )
            print("Patches may not work correctly!")
            print("Use -f/--force to apply patches anyway.")
            sys.exit(1)

    # Calculate screen parameters
    screen_size = (DISP_WIDTH * DISP_HEIGHT) // 8  # 9600 = 0x2580
    screen_distance = screen_size + 0x380  # 0x2900
    bytes_per_row = DISP_WIDTH // 8  # 30 for 240px

    # For 128KB RAM (0x20000), calculate screen_base using standard Mac address space
    # Original Mac screen is at top of memory (0x400000), we place framebuffer below it
    screen_base = 0x400000 - screen_distance  # 0x3FD700 for 240x320

    screen_base_l16 = screen_base & 0xFFFF

    print(f"Screen params: size={screen_size:#x}, distance={screen_distance:#x}")
    print(f"Screen base: {screen_base:#x}, base_l16: {screen_base_l16:#x}")
    print(f"Bytes per row: {bytes_per_row}")

    # 1. Disable checksum check
    rom_write16(rom, 0xD92, 0xB381)  # eor.l d1, d1

    # 2. Replace Sony driver
    rom[ROM_PLUSv3_SONYDRV : ROM_PLUSv3_SONYDRV + len(SONY_DRIVER)] = SONY_DRIVER
    # Set FaultyRegion address in last 4 bytes of driver
    rom_write32(rom, ROM_PLUSv3_SONYDRV + len(SONY_DRIVER) - 4, 0xC00069)

    # 3. Jump patch at 0x42-0x57 for screen setup
    rom_write16(rom, 0x42, 0x6000)  # bra
    rom_write16(rom, 0x44, 0x62 - 0x44)  # offset

    # Patch area at 0x46
    patch_0 = 0x46
    rom_write16(rom, patch_0 + 0, 0x9BFC)  # suba.l #imm32, A5
    rom_write16(rom, patch_0 + 2, 0)
    rom_write16(rom, patch_0 + 4, screen_distance)
    rom_write16(rom, patch_0 + 6, 0x6000)  # bra
    rom_write16(rom, patch_0 + 8, 0x3A4 - (patch_0 + 8))  # return to 0x3a4

    # 4. Screen resolution patches
    rom_write16(rom, 0x8C, screen_base_l16)
    rom_write16(rom, 0x148, screen_base_l16)
    rom_write32(
        rom,
        0x164,
        screen_coord(
            DISP_WIDTH // 2 - 24, DISP_HEIGHT // 2 + 8, screen_base, bytes_per_row
        ),
    )
    rom_write16(rom, 0x188, bytes_per_row)
    rom_write16(rom, 0x194, bytes_per_row)
    rom_write16(rom, 0x19C, (6 * bytes_per_row) - 1)
    rom_write32(
        rom,
        0x1A4,
        screen_coord(
            DISP_WIDTH // 2 - 8, DISP_HEIGHT // 2 + 16, screen_base, bytes_per_row
        ),
    )
    rom_write16(rom, 0x1EE, (screen_size // 4) - 1)

    # Icon positions (centered for 240x320)
    center_x = DISP_WIDTH // 2
    center_y = DISP_HEIGHT // 2
    rom_write32(
        rom,
        0xF0C,
        screen_coord(center_x - 16, center_y - 26, screen_base, bytes_per_row),
    )
    rom_write32(
        rom,
        0xF18,
        screen_coord(center_x - 8, center_y - 20, screen_base, bytes_per_row),
    )
    rom_write32(
        rom,
        0x7E0,
        screen_coord(center_x - 16, center_y - 26, screen_base, bytes_per_row),
    )
    rom_write32(
        rom,
        0x7F2,
        screen_coord(center_x - 8, center_y - 11, screen_base, bytes_per_row),
    )

    # 5. Jump to patch_0 at 0x3a0
    rom_write16(rom, 0x3A0, 0x6000)  # bra
    rom_write16(rom, 0x3A2, patch_0 - 0x3A2)

    # 6. More resolution patches
    rom_write16(rom, 0x474, bytes_per_row)
    rom_write16(rom, 0x494, DISP_HEIGHT)
    rom_write16(rom, 0x498, DISP_WIDTH)
    rom_write16(rom, 0xA0E, DISP_HEIGHT)
    rom_write16(rom, 0xA10, DISP_WIDTH)
    rom_write16(rom, 0xEE2, bytes_per_row - 4)
    rom_write16(rom, 0xEF2, bytes_per_row)
    rom_write16(rom, 0xF36, bytes_per_row - 2)

    # Cursor/showcursor patches
    rom[0x1CD1] = bytes_per_row  # hidecursor
    rom_write16(rom, 0x1D48, DISP_WIDTH - 32)
    rom_write16(rom, 0x1D4E, DISP_WIDTH - 32)
    rom_write16(rom, 0x1D6E, DISP_HEIGHT - 16)
    rom_write16(rom, 0x1D74, DISP_HEIGHT)
    rom[0x1D93] = bytes_per_row
    rom_write16(rom, 0x1E68, DISP_HEIGHT)
    rom_write16(rom, 0x1E6E, DISP_WIDTH)
    rom_write16(rom, 0x1E82, DISP_HEIGHT)

    print("Patches applied successfully!")
    return bytes(rom)


def main():
    import argparse

    parser = argparse.ArgumentParser(
        description="Generate patched ROM for CYD Mac Emulator"
    )
    parser.add_argument(
        "rom_path",
        nargs="?",
        default="rom.bin",
        help="Path to input ROM file (default: rom.bin)",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="rom_patched.bin",
        help="Output path for patched ROM (default: rom_patched.bin)",
    )
    parser.add_argument(
        "-f",
        "--force",
        action="store_true",
        help="Force patching even if ROM version doesn't match",
    )
    args = parser.parse_args()

    # Read original ROM
    try:
        with open(args.rom_path, "rb") as f:
            rom_data = f.read()
    except FileNotFoundError:
        print(f"Error: {args.rom_path} not found!")
        sys.exit(1)

    if len(rom_data) != ROM_SIZE:
        print(f"Error: ROM size {len(rom_data)} != expected {ROM_SIZE}")
        sys.exit(1)

    print(f"Loaded ROM: {len(rom_data)} bytes from {args.rom_path}")

    # Apply patches
    patched_rom = patch_rom(rom_data, force=args.force)

    # Write output
    with open(args.output, "wb") as f:
        f.write(patched_rom)

    print(f"\nPatched ROM written to: {args.output}")
    print(f"Size: {len(patched_rom)} bytes")

    # Verify first few bytes
    print(f"\nFirst 16 bytes of patched ROM:")
    for i in range(0, 16, 4):
        print(
            f"  {i:04x}: {patched_rom[i]:02x} {patched_rom[i + 1]:02x} {patched_rom[i + 2]:02x} {patched_rom[i + 3]:02x}"
        )


if __name__ == "__main__":
    main()
