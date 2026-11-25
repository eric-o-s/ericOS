#!/usr/bin/env python3
"""Sanity-check a .uf2 before it gets copied onto the Pico.

An RP2350 image from picotool is a composite of two segments:
  * an optional ARM "safest" image at the top of flash (0x10ffff00), and
  * the flash-resident image at 0x10000000 (RISC-V boot2 + the app).
Both are legal on their own, so the check validates each segment's UF2 framing
and address range rather than insisting on a single family.
"""
import struct
import sys

UF2_MAGIC0 = 0x0A324655
UF2_MAGIC1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
FAMILIES = {
    0xE48BFF56: "RP2040",
    0xE48BFF57: "RP2350_ARM",
    0xE48BFF59: "RP2350_RISCV",
}
FLASH_BASE = 0x10000000
# The RP2350 XIP window is 16 MB wide; 4 MB of real flash is mirrored there, and
# the "safest" region shows up near the top of that window.
XIP_END = FLASH_BASE + (16 << 20)
SAFE_ARM = 0x10FFFF00


def main(path):
    data = open(path, "rb").read()
    if len(data) % 512:
        print("FAIL: %s is %d bytes, not a multiple of 512" % (path, len(data)))
        return 1
    total = len(data) // 512
    if not total:
        print("FAIL: %s is empty" % path)
        return 1

    segments = {}
    for index in range(total):
        start = index * 512
        m0, m1, flags, addr, payload, block_no, num_blocks, family = struct.unpack(
            "<8I", data[start:start + 32])
        magic_end = struct.unpack("<I", data[start + 508:start + 512])[0]
        label = "%s block %d" % (path, index)
        if m0 != UF2_MAGIC0 or m1 != UF2_MAGIC1:
            print("FAIL: %s has bad UF2 magic" % label)
            return 1
        if magic_end != UF2_MAGIC_END:
            print("FAIL: %s has bad magicEnd 0x%08x" % (label, magic_end))
            return 1
        if family not in FAMILIES:
            print("FAIL: %s has unknown familyID 0x%08x" % (label, family))
            return 1
        if payload != 256:
            print("FAIL: %s payload is %d, expected 256" % (label, payload))
            return 1
        if flags & 0x00002000 == 0:
            print("FAIL: %s flags 0x%05x omit the familyID-present bit" % (label, flags))
            return 1
        if not FLASH_BASE <= addr or addr + payload > XIP_END:
            print("FAIL: %s targets 0x%08x, outside RP2350 flash" % (label, addr))
            return 1
        # Blocks are grouped by declared image size; each image numbers from 0.
        segments.setdefault((num_blocks, addr >= SAFE_ARM), []).append((index, block_no, addr, family))

    for (num_blocks, is_safe), blocks in segments.items():
        blocks.sort()
        if len(blocks) != num_blocks:
            print("FAIL: image with numBlocks=%d has %d blocks present" % (num_blocks, len(blocks)))
            return 1
        for position, (index, block_no, _addr, _family) in enumerate(blocks):
            if block_no != position:
                print("FAIL: block %d declares index %d, expected %d" % (index, block_no, position))
                return 1
        low = blocks[0][2]
        high = blocks[-1][2] + 256
        for position in range(1, len(blocks)):
            if blocks[position][2] != blocks[position - 1][2] + 256:
                print("FAIL: address gap before block %d (0x%08x after 0x%08x)"
                      % (blocks[position][0], blocks[position][2], blocks[position - 1][2]))
                return 1
        families = {f for _i, _n, _a, f in blocks}
        print("  segment %-14s %5d blocks  0x%08x..0x%08x  %s"
              % ("ARM-safest" if is_safe else "flash", num_blocks, low, high,
                 ",".join(sorted(FAMILIES[f] for f in families))))

    print("OK: %s (%d blocks, %d bytes payload)" % (path, total, total * 256))
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: %s <file.uf2>" % sys.argv[0])
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
