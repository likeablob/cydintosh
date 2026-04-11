/*
 * Copyright 2024 Matt Evans
 *
 * Portions of the {READ,WRITE}_{BYTE,WORD,LONG} macros are from
 * Musashi, Copyright 1998-2002 Karl Stenerud.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef MACHW_H
#define MACHW_H

#include "rom.h"

#define ROM_ADDR      0x400000
#define RAM_SIZE      (1024 * UMAC_MEMSIZE)
#define RAM_HIGH_ADDR 0x600000

#define PV_SONY_ADDR 0xc00069

////////////////////////////////////////////////////////////////////////////////
// RAM accessors

extern uint8_t *_ram_base;
extern uint8_t *_rom_base;
extern uint8_t *_fb_base; // Separate framebuffer in internal RAM

// Framebuffer region: offset and size
#define FB_SIZE       (DISP_WIDTH * DISP_HEIGHT / 8)
#define FB_START      (RAM_SIZE - (FB_SIZE + 0x380))
#define FB_END        (FB_START + FB_SIZE)
#define IS_FB_ADDR(x) ((x) >= FB_START && (x) < FB_END)
#define FB_OFFSET(x)  ((x) - FB_START)

static inline uint8_t *ram_get_base(void) {
    return _ram_base;
}

static inline uint8_t *rom_get_base(void) {
    return _rom_base;
}

static inline uint8_t *fb_get_base(void) {
    return _fb_base;
}

extern int overlay;

#define ADR24(x) ((x) & 0xffffff)

#define IS_ROM(x) (((ADR24(x) & 0xf00000) == ROM_ADDR) || (overlay && (ADR24(x) & 0xf00000) == 0))
#define IS_RAM(x) \
    ((!overlay && ((ADR24(x) & 0xc00000) == 0)) || ((ADR24(x) & 0xe00000) == RAM_HIGH_ADDR))

#define CLAMP_RAM_ADDR(x) ((x) >= RAM_SIZE ? (x) % RAM_SIZE : (x))

#define IS_VIA(x)    ((ADR24(x) & 0xe80000) == 0xe80000)
#define IS_IWM(x)    ((ADR24(x) >= 0xdfe1ff) && (ADR24(x) < (0xdfe1ff + 0x2000)))
#define IS_SCC_RD(x) ((ADR24(x) & 0xf00000) == 0x900000)
#define IS_SCC_WR(x) ((ADR24(x) & 0xf00000) == 0xb00000)
#define IS_DUMMY(x) \
    (((ADR24(x) >= 0x800000) && (ADR24(x) < 0x9ffff8)) || ((ADR24(x) & 0xf00000) == 0x500000))
#define IS_TESTSW(x)  (ADR24(x) >= 0xf00000)
#define IS_ESP_IPC(x) ((ADR24(x) >= 0xF00000) && (ADR24(x) < 0xF000100))

/* Unaligned/BE read/write macros from Mushashi: */
#define READ_BYTE(BASE, ADDR) (BASE)[ADDR]
#define READ_WORD(BASE, ADDR) (((BASE)[ADDR] << 8) | (BASE)[(ADDR) + 1])
#define READ_LONG(BASE, ADDR)                                                        \
    (((BASE)[ADDR] << 24) | ((BASE)[(ADDR) + 1] << 16) | ((BASE)[(ADDR) + 2] << 8) | \
     (BASE)[(ADDR) + 3])
#define READ_WORD_AL(BASE, ADDR) (__builtin_bswap16(*(uint16_t *)&(BASE)[(ADDR)]))

#define WRITE_BYTE(BASE, ADDR, VAL)  \
    do {                             \
        (BASE)[ADDR] = (VAL) & 0xff; \
    } while (0)
#define WRITE_WORD(BASE, ADDR, VAL)         \
    do {                                    \
        (BASE)[ADDR] = ((VAL) >> 8) & 0xff; \
        (BASE)[(ADDR) + 1] = (VAL) & 0xff;  \
    } while (0)
#define WRITE_LONG(BASE, ADDR, VAL)                \
    do {                                           \
        (BASE)[ADDR] = ((VAL) >> 24) & 0xff;       \
        (BASE)[(ADDR) + 1] = ((VAL) >> 16) & 0xff; \
        (BASE)[(ADDR) + 2] = ((VAL) >> 8) & 0xff;  \
        (BASE)[(ADDR) + 3] = (VAL) & 0xff;         \
    } while (0)

/* Specific RAM/ROM access: */

// Framebuffer accesses go to separate internal RAM buffer
#define RAM_RD8(addr) \
    (IS_FB_ADDR(addr) ? READ_BYTE(_fb_base, FB_OFFSET(addr)) : READ_BYTE(_ram_base, addr))
#define RAM_RD16(addr) \
    (IS_FB_ADDR(addr) ? READ_WORD(_fb_base, FB_OFFSET(addr)) : READ_WORD(_ram_base, addr))
#define RAM_RD_ALIGNED_BE16(addr) \
    (IS_FB_ADDR(addr) ? READ_WORD_AL(_fb_base, FB_OFFSET(addr)) : READ_WORD_AL(_ram_base, addr))
#define RAM_RD32(addr) \
    (IS_FB_ADDR(addr) ? READ_LONG(_fb_base, FB_OFFSET(addr)) : READ_LONG(_ram_base, addr))

#define RAM_WR8(addr, val)                              \
    do {                                                \
        if (IS_FB_ADDR(addr))                           \
            WRITE_BYTE(_fb_base, FB_OFFSET(addr), val); \
        else                                            \
            WRITE_BYTE(_ram_base, addr, val);           \
    } while (0)
#define RAM_WR16(addr, val)                             \
    do {                                                \
        if (IS_FB_ADDR(addr))                           \
            WRITE_WORD(_fb_base, FB_OFFSET(addr), val); \
        else                                            \
            WRITE_WORD(_ram_base, addr, val);           \
    } while (0)
#define RAM_WR32(addr, val)                             \
    do {                                                \
        if (IS_FB_ADDR(addr))                           \
            WRITE_LONG(_fb_base, FB_OFFSET(addr), val); \
        else                                            \
            WRITE_LONG(_ram_base, addr, val);           \
    } while (0)

#define ROM_RD8(addr)             READ_BYTE(_rom_base, addr)
#define ROM_RD16(addr)            READ_WORD(_rom_base, addr)
#define ROM_RD_ALIGNED_BE16(addr) READ_WORD_AL(_rom_base, addr)
#define ROM_RD32(addr)            READ_LONG(_rom_base, addr)

#endif