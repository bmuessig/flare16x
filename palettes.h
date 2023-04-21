//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// palettes.h: Header file for the palettes and palette functions
//

#ifndef FLARE16X_PALETTES_H
#define FLARE16X_PALETTES_H

#include <stdint.h>

// Make sure the palette struct aligns to a 32-bit dword boundary
#pragma pack(push, 4)

// Represents a palette value
typedef struct {
    // Relative to a scale from 0x00 to 0xff, where does the palette value start
    uint8_t base;
    // The number of consecutive colors (the resolution of the palette value)
    uint8_t width;
    // The RGB565 color used to represent this palette value
    uint16_t color;
} flare16x_palette_entry;

// Restore regular 32 or 64 bit struct boundaries
#pragma pack(pop)

// List of palettes used for detection
enum {
    FLARE16X_PALETTES_UNKNOWN,
    FLARE16X_PALETTES_IRON,
    FLARE16X_PALETTES_GRAYSCALE,
    FLARE16X_PALETTES_RAINBOW,
    FLARE16X_PALETTES_COUNT = FLARE16X_PALETTES_RAINBOW,
    FLARE16X_PALETTES_MIN = FLARE16X_PALETTES_IRON,
    FLARE16X_PALETTES_MAX = FLARE16X_PALETTES_RAINBOW
};

// The size of the palette cache used in some functions
#define FLARE16X_PALETTES_CACHE_SIZE 4

// Constant that can be used to signal infinite possible error
#define FLARE16X_PALETTES_IGNORE_ERRORS 0xffff

// Represents a palette color cache
typedef struct {
    flare16x_palette_entry entries[FLARE16X_PALETTES_CACHE_SIZE];
    uint8_t length;
    uint8_t index;
} flare16x_palette_cache;

// The raw iron palette struct data
extern const flare16x_palette_entry palette_iron[];
// The number of struct elements in the iron palette
extern const uint8_t palette_iron_count;

// The grayscale palette data
extern const flare16x_palette_entry palette_grayscale[];
// The number of struct elements in the grayscale palette
extern const uint8_t palette_grayscale_count;

// The raw rainbow palette struct data
extern const flare16x_palette_entry palette_rainbow[];
// The number of struct elements in the rainbow palette
extern const uint8_t palette_rainbow_count;

// Returns the palette that belongs to the supplied enum index value or NULL, if it could not be found
const flare16x_palette_entry* flare16x_palettes_get(uint8_t palette_index);

// Returns the length of the palette that belongs to the supplied enum index value or 0 if it could not be found
int flare16x_palettes_get_length(uint8_t palette_index);

// Initializes the given cache struct
// This will not leak any memory if done repeatedly
flare16x_error flare16x_palettes_cache_init(flare16x_palette_cache* cache);

// Attempts to find the color in the cache or in the given palette and updates the cache
// Once a cache is used it should not be used with another palette to prevent false-positives
// To switch to another palette, reinitialize the cache first
flare16x_error flare16x_palettes_find_color(uint16_t color, uint8_t palette_index, flare16x_palette_cache* cache,
                                            const flare16x_palette_entry** result_entry);

// Attempts to find the value in the cache or in the given palette and updates the cache
// Once a cache is used it should not be used with another palette to prevent false-positives
// To switch to another palette, reinitialize the cache first
flare16x_error flare16x_palettes_find_value(uint8_t value, uint8_t palette_index, flare16x_palette_cache* cache,
                                            const flare16x_palette_entry** result_entry);

// Analyzes the canvas and returns the matching palette enum index
flare16x_error flare16x_palettes_determine(flare16x_canvas* canvas, uint16_t max_errors, uint8_t* palette_index);

#endif //FLARE16X_PALETTES_H
