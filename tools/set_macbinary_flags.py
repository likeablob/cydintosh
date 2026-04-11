#!/usr/bin/env python3
"""
Set Finder flags in MacBinary files.

Finder flags (fdFlags) are stored at offset 73 in MacBinary header.

Usage:
    python3 set_macbinary_flags.py <file.bin> [+bndl] [+custom] [+inited]
    python3 set_macbinary_flags.py <file.bin> -show
"""

import sys
from pathlib import Path

HFS_FNDR_ISONDESK = 0x0001
HFS_FNDR_HASBEENINITED = 0x0100
HFS_FNDR_HASCUSTOMICON = 0x0400
HFS_FNDR_HASBUNDLE = 0x2000
HFS_FNDR_ISINVISIBLE = 0x4000

FLAG_NAMES = {
    "bndl": HFS_FNDR_HASBUNDLE,
    "custom": HFS_FNDR_HASCUSTOMICON,
    "inited": HFS_FNDR_HASBEENINITED,
    "invisible": HFS_FNDR_ISINVISIBLE,
}


def read_be_u16(data, offset):
    return (data[offset] << 8) | data[offset + 1]


def write_be_u16(data, offset, value):
    data[offset] = (value >> 8) & 0xFF
    data[offset + 1] = value & 0xFF


def get_finder_flags(data):
    """Get Finder flags from MacBinary header at offset 73."""
    if len(data) < 128:
        raise ValueError("File too small to be MacBinary")
    return read_be_u16(data, 73)


def calc_macbinary_crc(data):
    """Calculate MacBinary II CRC-16-CCITT over bytes 0-123."""
    crc = 0
    for byte in data[:124]:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
            crc &= 0xFFFF
    return crc


def update_crc(data):
    """Update CRC at offset 124-125."""
    crc = calc_macbinary_crc(data)
    write_be_u16(data, 124, crc)


def set_finder_flags(data, flags):
    """Set Finder flags in MacBinary header at offset 73."""
    if len(data) < 128:
        raise ValueError("File too small to be MacBinary")
    write_be_u16(data, 73, flags)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    filepath = Path(sys.argv[1])
    if not filepath.exists():
        print(f"Error: File not found: {filepath}")
        sys.exit(1)

    data = bytearray(filepath.read_bytes())

    # Verify MacBinary
    if data[0] != 0:
        print(f"Warning: First byte is {data[0]}, expected 0 for MacBinary")

    filename_len = data[1]
    filename = data[2 : 2 + filename_len].decode("mac_roman", errors="replace")

    file_type = data[65:69].decode("ascii", errors="replace")
    file_creator = data[69:73].decode("ascii", errors="replace")

    current_flags = get_finder_flags(data)

    print(f"File: {filepath}")
    print(f"Name: {filename}")
    print(f"Type: {file_type}, Creator: {file_creator}")

    if len(sys.argv) < 3 or sys.argv[2] == "-show":
        flags_desc = []
        if current_flags & HFS_FNDR_HASBUNDLE:
            flags_desc.append("bndl")
        if current_flags & HFS_FNDR_HASCUSTOMICON:
            flags_desc.append("custom")
        if current_flags & HFS_FNDR_HASBEENINITED:
            flags_desc.append("inited")
        if current_flags & HFS_FNDR_ISINVISIBLE:
            flags_desc.append("invisible")
        print(
            f"Finder flags: 0x{current_flags:04x} ({', '.join(flags_desc) or 'none'})"
        )
        return

    flags_to_set = 0
    for arg in sys.argv[2:]:
        if arg.startswith("+"):
            flag_name = arg[1:].lower()
            if flag_name in FLAG_NAMES:
                flags_to_set |= FLAG_NAMES[flag_name]
            else:
                print(f"Unknown flag: {arg}")
                print(f"Available flags: +bndl, +custom, +inited, +invisible")
                sys.exit(1)

    if flags_to_set == 0:
        flags_to_set = HFS_FNDR_HASBUNDLE | HFS_FNDR_HASBEENINITED

    new_flags = current_flags | flags_to_set
    set_finder_flags(data, new_flags)
    update_crc(data)

    filepath.write_bytes(data)

    print(f"Finder flags: 0x{current_flags:04x} -> 0x{new_flags:04x}")
    print("Flags set:")
    if flags_to_set & HFS_FNDR_HASBUNDLE:
        print("  + HASBUNDLE")
    if flags_to_set & HFS_FNDR_HASCUSTOMICON:
        print("  + HASCUSTOMICON")
    if flags_to_set & HFS_FNDR_HASBEENINITED:
        print("  + HASBEENINITED")
    if flags_to_set & HFS_FNDR_ISINVISIBLE:
        print("  + ISINVISIBLE")


if __name__ == "__main__":
    main()
