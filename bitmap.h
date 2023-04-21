//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// bitmap.h: Header file for bitmap read/write
//

#ifndef FLARE16X_BITMAP_H
#define FLARE16X_BITMAP_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "error.h"
#include "canvas.h"

// Bitmaps consist of three or four different fields:
// File header, image header, color table and image data

// For 16-bit RGB565 images it looks like this:
// File header, image header, uint32 red_mask, green_mask, blue_mask, image data

// Import works with fixed-size RGB565 images, while export works with fixed size RGB888 images

// The maximum pixel count used to prevent external DOS attacks through malformed DIB headers
#define FLARE16X_BITMAP_MAX_PIXELS (1 << 24)

// Constants for the header magic and reserved fields
#define FLARE16X_BITMAP_HEADER_MAGIC 0x4d42u
#define FLARE16X_BITMAP_HEADER_RESERVED 0x0u

// Make sure the standardised bitmap structs align to 16-bit word boundaries
#pragma pack(push, 2)

// The bitmap header struct
typedef struct {
    uint16_t magic; // must be "BM"
    uint32_t file_size; // is 0x12b52 for TG16x images
    uint32_t reserved; // must be 0
    uint32_t payload_offset; // must be 0x42 for RGB565 // 0x36 für rgb888
} flare16x_bitmap_header;

// The bitmap DIB struct
typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes; // must be 1
    uint16_t bit_count; // must be 16 or 24
    uint32_t compression; // must be 0x3 or RGB
    uint32_t size_image;
    int32_t hor_px_per_meter; // dnc
    int32_t ver_px_per_meter; // dnc
    uint32_t colors_used; // must be 0x0
    uint32_t colors_important; // must be 0x0
} flare16x_bitmap_dib;

// The RGB565 mask struct
typedef struct {
    uint32_t mask_red; // must be 0xf800
    uint32_t mask_green; // must be 0x7e0
    uint32_t mask_blue; // must be 0x1f
} flare16x_bitmap_mask;

// Restore regular 32 or 64 bit struct boundaries
#pragma pack(pop)

// Constants for the DIB compression field
enum {
    FLARE16X_BITMAP_COMPRESSION_RGB = 0x0u,
    FLARE16X_BITMAP_COMPRESSION_BITFIELDS = 0x3u
};

// RGB565 constants for the masks struct
enum {
    FLARE16X_BITMAP_MASK_RGB565_RED = 0xf800u,
    FLARE16X_BITMAP_MASK_RGB565_GREEN = 0x7e0u,
    FLARE16X_BITMAP_MASK_RGB565_BLUE = 0x1fu
};

// Universal bitmap struct
typedef struct {
    // The pointer to the bitmap header
    flare16x_bitmap_header* header;
    // The pointer to the DIB header
    flare16x_bitmap_dib* dib;
    // The size of the DIB header
    size_t dib_size;
    // The RGB565 bit mask
    flare16x_bitmap_mask* mask;
    // The size of the mask struct
    size_t mask_size;
    // The pointer to the pixel data
    union {
        uint8_t* pixels;
        uint16_t* pixels565;
        uint32_t* pixels8888;
    };
    // The size of the pixel data
    size_t pixels_size;
    // The aligned data width of one scanline
    uint16_t stride;
} flare16x_bitmap;

// Creates a new 16-bit RGB565 bitmap, allocates the required memory and fills in the values
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_bitmap_create16(uint16_t width, uint16_t height, flare16x_bitmap* bitmap);

// Creates a new 24-bit RGB888 bitmap, allocates the required memory and fills in the values
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_bitmap_create24(uint16_t width, uint16_t height, flare16x_bitmap* bitmap);

// Creates a new 32-bit RGBA8888 bitmap, allocates the required memory and fills in the values
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_bitmap_create32(uint16_t width, uint16_t height, flare16x_bitmap* bitmap);

// Attempts to load a bitmap from file
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_bitmap_load(FILE* bitmap_file, flare16x_bitmap* bitmap_struct);

// Attempts to store a bitmap to file
flare16x_error flare16x_bitmap_store(flare16x_bitmap* bitmap_struct, FILE* bitmap_file);

// Destroys an existing bitmap context and frees the resources
flare16x_error flare16x_bitmap_destroy(flare16x_bitmap* bitmap);

// Copies a region from a bitmap buffer to a canvas buffer
// Will overwrite any existing canvas state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_bitmap_edit(flare16x_bitmap* bitmap, uint16_t offset_x, uint16_t offset_y,
        uint16_t width, uint16_t height, flare16x_canvas* canvas);

// Copies a region from a canvas buffer to a bitmap buffer
flare16x_error flare16x_bitmap_merge(flare16x_canvas* canvas, uint16_t offset_x, uint16_t offset_y,
        flare16x_bitmap* bitmap);

#endif //FLARE16X_BITMAP_H
