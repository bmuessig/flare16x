//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// palette_grayscale.c: Contains the grayscale palette
//

#include <stdint.h>

// The number of struct elements in the grayscale palette
const uint8_t palette_grayscale_count = 64;

// The raw grayscale palette struct data
const uint8_t palette_grayscale[256] =
    {
        0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x20, 0x00, 0x08, 0x04, 0x41, 0x08, 0x0c, 0x04, 0x61, 0x08,
        0x10, 0x04, 0x82, 0x10, 0x14, 0x04, 0xa2, 0x10, 0x18, 0x04, 0xc3, 0x18, 0x1c, 0x04, 0xe3, 0x18,
        0x20, 0x04, 0x04, 0x21, 0x24, 0x04, 0x24, 0x21, 0x28, 0x04, 0x45, 0x29, 0x2c, 0x04, 0x65, 0x29,
        0x30, 0x04, 0x86, 0x31, 0x34, 0x04, 0xa6, 0x31, 0x38, 0x04, 0xc7, 0x39, 0x3c, 0x04, 0xe7, 0x39,
        0x40, 0x04, 0x08, 0x42, 0x44, 0x04, 0x28, 0x42, 0x48, 0x04, 0x49, 0x4a, 0x4c, 0x04, 0x69, 0x4a,
        0x50, 0x04, 0x8a, 0x52, 0x54, 0x04, 0xaa, 0x52, 0x58, 0x04, 0xcb, 0x5a, 0x5c, 0x04, 0xeb, 0x5a,
        0x60, 0x04, 0x0c, 0x63, 0x64, 0x04, 0x2c, 0x63, 0x68, 0x04, 0x4d, 0x6b, 0x6c, 0x04, 0x6d, 0x6b,
        0x70, 0x04, 0x8e, 0x73, 0x74, 0x04, 0xae, 0x73, 0x78, 0x04, 0xcf, 0x7b, 0x7c, 0x04, 0xef, 0x7b,
        0x80, 0x04, 0x10, 0x84, 0x84, 0x04, 0x30, 0x84, 0x88, 0x04, 0x51, 0x8c, 0x8c, 0x04, 0x71, 0x8c,
        0x90, 0x04, 0x92, 0x94, 0x94, 0x04, 0xb2, 0x94, 0x98, 0x04, 0xd3, 0x9c, 0x9c, 0x04, 0xf3, 0x9c,
        0xa0, 0x04, 0x14, 0xa5, 0xa4, 0x04, 0x34, 0xa5, 0xa8, 0x04, 0x55, 0xad, 0xac, 0x04, 0x75, 0xad,
        0xb0, 0x04, 0x96, 0xb5, 0xb4, 0x04, 0xb6, 0xb5, 0xb8, 0x04, 0xd7, 0xbd, 0xbc, 0x04, 0xf7, 0xbd,
        0xc0, 0x04, 0x18, 0xc6, 0xc4, 0x04, 0x38, 0xc6, 0xc8, 0x04, 0x59, 0xce, 0xcc, 0x04, 0x79, 0xce,
        0xd0, 0x04, 0x9a, 0xd6, 0xd4, 0x04, 0xba, 0xd6, 0xd8, 0x04, 0xdb, 0xde, 0xdc, 0x04, 0xfb, 0xde,
        0xe0, 0x04, 0x1c, 0xe7, 0xe4, 0x04, 0x3c, 0xe7, 0xe8, 0x04, 0x5d, 0xef, 0xec, 0x04, 0x7d, 0xef,
        0xf0, 0x04, 0x9e, 0xf7, 0xf4, 0x04, 0xbe, 0xf7, 0xf8, 0x04, 0xdf, 0xff, 0x00, 0x04, 0xff, 0xff
    };
