//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// palettes.c: Handles the palettes and palette functions
//

#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "locator.h"
#include "canvas.h"

#include "palettes.h"

// Returns the palette that belongs to the supplied enum index value or NULL, if it could not be found
const flare16x_palette_entry* flare16x_palettes_get(uint8_t palette_index)
{
    switch (palette_index)
    {
        case FLARE16X_PALETTES_IRON:
            return palette_iron;
        case FLARE16X_PALETTES_GRAYSCALE:
            return palette_grayscale;
        case FLARE16X_PALETTES_RAINBOW:
            return palette_rainbow;
        default:
            return NULL;
    }
}

// Returns the length of the palette that belongs to the supplied enum index value or 0 if it could not be found
int flare16x_palettes_get_length(uint8_t palette_index)
{
    switch (palette_index)
    {
        case FLARE16X_PALETTES_IRON:
            return palette_iron_count;
        case FLARE16X_PALETTES_GRAYSCALE:
            return palette_grayscale_count;
        case FLARE16X_PALETTES_RAINBOW:
            return palette_rainbow_count;
        default:
            return 0;
    }
}

// Initializes the given cache struct
// This will not leak any memory if done repeatedly
flare16x_error flare16x_palettes_cache_init(flare16x_palette_cache* cache)
{
    // Make sure that the cache pointer is not null
    if (cache == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_PALETTES);

    // Zero the structure
    memset(cache, 0, sizeof(flare16x_palette_cache));

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_PALETTES);
}

// Attempts to find the color in the cache or in the given palette and updates the cache
// Once a cache is used it should not be used with another palette to prevent false-positives
// To switch to another palette, reinitialize the cache first
flare16x_error flare16x_palettes_find_color(uint16_t color, uint8_t palette_index, flare16x_palette_cache* cache,
                                            const flare16x_palette_entry** result_entry)
{
    // Make sure that the cache and result pointers are not null
    if (cache == NULL || result_entry == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_PALETTES);

    // Validate the palette index and fetch the palette pointer
    const flare16x_palette_entry* palette = flare16x_palettes_get(palette_index);
    int palette_length = flare16x_palettes_get_length(palette_index);
    if (palette == NULL || palette_length < 1)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_PALETTES);

    // Check the cache first
    int item;
    for (item = 0; item < FLARE16X_PALETTES_CACHE_SIZE; item++)
        if (cache->entries[item].color == color)
        {
            // Assign the result pointer
            *result_entry = &cache->entries[item];
            return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_PALETTES);
        }

    // Iterate through the palette and try to find the value
    for (item = 0; item < palette_length; item++)
        if (palette[item].color == color)
        {
            // Assign the result pointer
            *result_entry = &palette[item];

            // Check, if a new item has to be added to the cache
            if (cache->length < FLARE16X_PALETTES_CACHE_SIZE)
            {
                // Copy the new palette item, reset its pointer and increment the size
                cache->index = 0;
                memcpy(&cache->entries[cache->length++], palette + item, sizeof(flare16x_palette_entry));
                return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_PALETTES);
            }

            // If the palette is already full, update the item at the pointer
            memcpy(&cache->entries[cache->index++],palette + item, sizeof(flare16x_palette_entry));

            // Finally, make sure the pointer is in range
            if (cache->index >= cache->length)
                cache->index = 0;

            return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_PALETTES);
        }

    // The item was not found
    *result_entry = NULL;
    return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_PALETTES);
}

// Attempts to find the value in the cache or in the given palette and updates the cache
// Once a cache is used it should not be used with another palette to prevent false-positives
// To switch to another palette, reinitialize the cache first
flare16x_error flare16x_palettes_find_value(uint8_t value, uint8_t palette_index, flare16x_palette_cache* cache,
                                            const flare16x_palette_entry** result_entry)
{
    // Make sure that the cache and result pointers are not null
    if (cache == NULL || result_entry == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_PALETTES);

    // Validate the palette index and fetch the palette pointer
    const flare16x_palette_entry* palette = flare16x_palettes_get(palette_index);
    int palette_length = flare16x_palettes_get_length(palette_index);
    if (palette == NULL || palette_length < 1)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_PALETTES);

    // Check the cache first
    int item;
    for (item = 0; item < FLARE16X_PALETTES_CACHE_SIZE; item++)
        if (cache->entries[item].base <= value && cache->entries[item].base + cache->entries[item].width > value)
        {
            // Assign the result pointer
            *result_entry = &cache->entries[item];
            return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_PALETTES);
        }

    // Iterate through the palette and try to find the value
    for (item = 0; item < palette_length; item++)
        if (palette[item].base <= value && palette[item].base + palette[item].width > value)
        {
            // Assign the result pointer
            *result_entry = &palette[item];

            // Check, if a new item has to be added to the cache
            if (cache->length < FLARE16X_PALETTES_CACHE_SIZE)
            {
                // Copy the new palette item, reset its pointer and increment the size
                cache->index = 0;
                memcpy(&cache->entries[cache->length++], palette + item, sizeof(flare16x_palette_entry));
                return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_PALETTES);
            }

            // If the palette is already full, update the item at the pointer
            memcpy(&cache->entries[cache->index++],palette + item, sizeof(flare16x_palette_entry));

            // Finally, make sure the pointer is in range
            if (cache->index >= cache->length)
                cache->index = 0;

            return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_PALETTES);
        }

    // The item was not found
    *result_entry = NULL;
    return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_PALETTES);
}

// Analyzes the canvas and returns the matching palette enum index
flare16x_error flare16x_palettes_determine(flare16x_canvas* canvas, uint16_t max_errors, uint8_t* palette_index)
{
    // Make sure the canvas and palette index pointers are not null
    if (canvas == NULL || canvas->pixels == NULL || palette_index == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_PALETTES);

    // Also verify width and height
    if (canvas->width == 0 || canvas->height == 0)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_PALETTES);

    // Allocate an array with the size of the number of palettes
    // This array keeps track of the number of color matches
    uint32_t palettes_counts[FLARE16X_PALETTES_COUNT];
    memset(palettes_counts, 0, FLARE16X_PALETTES_COUNT * sizeof(uint32_t));

    // Also, for speed, cache a number of the latest found palette entries from all palettes
    flare16x_palette_cache cache[FLARE16X_PALETTES_COUNT];
    memset(cache, 0, FLARE16X_PALETTES_COUNT * sizeof(flare16x_palette_cache));

    // Iterate through all pixels
    int x, y;
    for (y = 0; y < canvas->height; y++)
        for (x = 0; x < canvas->width; x++)
        {
            // Fetch the pixel first
            uint16_t p = flare16x_canvas_raw(x, y, canvas);

            // Make sure that the color is not used in the crosshair
            if (p == FLARE16X_LOCATOR_CROSSHAIR_BORDER || p == FLARE16X_LOCATOR_CROSSHAIR_FILL)
                continue;

            // Iterate through the palettes
            int current_palette, matching_palette = FLARE16X_PALETTES_UNKNOWN;
            for (current_palette = FLARE16X_PALETTES_MIN; current_palette <= FLARE16X_PALETTES_MAX; current_palette++)
            {
                // Try to look it up in the cache first
                int item;
                for (item = 0; item < cache[current_palette - FLARE16X_PALETTES_MIN].length; item++)
                    if (cache[current_palette - FLARE16X_PALETTES_MIN].entries[item].color == p)
                    {
                        palettes_counts[current_palette - FLARE16X_PALETTES_MIN]++;
                        matching_palette = current_palette;
                        break;
                    }

                // Check, if the item was found
                if (matching_palette != FLARE16X_PALETTES_UNKNOWN)
                    continue;

                // Otherwise fetch the palette pointer and assert that the palette must never be null
                const flare16x_palette_entry* palette = flare16x_palettes_get(current_palette);
                if (palette == NULL)
                    return flare16x_error_make(FLARE16X_ERROR_ASSERT, FLARE16X_ERROR_SOURCE_PALETTES);

                // Then iterate through the entire palette
                for (item = 0; item < flare16x_palettes_get_length(current_palette); item++)
                    if (palette[item].color == p)
                    {
                        // Increment the palette item
                        palettes_counts[current_palette - FLARE16X_PALETTES_MIN]++;
                        matching_palette = current_palette;

                        // Check, if a new item has to be added
                        if (cache[current_palette - FLARE16X_PALETTES_MIN].length < FLARE16X_PALETTES_CACHE_SIZE)
                        {
                            // Copy the new palette item, reset its pointer and increment the size
                            cache[current_palette - FLARE16X_PALETTES_MIN].index = 0;
                            memcpy(&cache[current_palette - FLARE16X_PALETTES_MIN].entries[
                                    cache[current_palette - FLARE16X_PALETTES_MIN].length++],
                                            palette + item, sizeof(flare16x_palette_entry));
                            break;
                        }

                        // If the palette is already full, update the item at the pointer
                        memcpy(&cache[current_palette - FLARE16X_PALETTES_MIN].entries[
                                cache[current_palette - FLARE16X_PALETTES_MIN].index++], palette + item,
                                        sizeof(flare16x_palette_entry));

                        // Finally, make sure the pointer is in range
                        if (cache[current_palette - FLARE16X_PALETTES_MIN].index >=
                            cache[current_palette - FLARE16X_PALETTES_MIN].length)
                            cache[current_palette - FLARE16X_PALETTES_MIN].index = 0;
                        break;
                    }
            }

            // Make sure an item was found
            if (matching_palette == FLARE16X_PALETTES_UNKNOWN)
            {
                // Only count down the maximum errors, if it is not IGNORE_ERRORS
                if (max_errors == FLARE16X_PALETTES_IGNORE_ERRORS)
                    continue;

                // Decrement the remaining maximum errors
                max_errors--;

                // If there are no more mishaps possible, fail
                if (max_errors < 1)
                    return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_PALETTES);
            }
        }

    // Now, determine the highest ranked palette
    int current_palette, highest_palette = FLARE16X_PALETTES_UNKNOWN, equal_palette = FLARE16X_PALETTES_UNKNOWN,
        highest_count = 0;
    for (current_palette = 0; current_palette < FLARE16X_PALETTES_COUNT; current_palette++)
    {
        if (highest_count < palettes_counts[current_palette])
        {
            highest_count = palettes_counts[current_palette];
            highest_palette = current_palette + FLARE16X_PALETTES_MIN;
        } else if (highest_count == palettes_counts[current_palette])
            equal_palette = current_palette + FLARE16X_PALETTES_MIN;
    }

    // Assign the result
    *palette_index = highest_palette;

    // Make sure there is only one highest palette
    if (highest_palette == FLARE16X_PALETTES_UNKNOWN || highest_palette == equal_palette)
        return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_PALETTES);

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_PALETTES);
}

