//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// canvas.c: Canvas image manipulation functions
//

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "error.h"

#include "canvas.h"

// Creates a new canvas and allocates the required memory
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_canvas_create(uint16_t width, uint16_t height, flare16x_canvas* canvas)
{
    // Make sure the canvas is not null
    if (canvas == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_CANVAS);

    // Verify width and height
    if (width == 0 || height == 0)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_CANVAS);

    // Copy width and height
    canvas->width = width;
    canvas->height = height;

    // Allocate the required memory
    canvas -> pixels = malloc(height * width * sizeof(uint16_t));
    if (canvas->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_CANVAS);

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_CANVAS);
}

// Copies a canvas region and creates a new canvas from it
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_canvas_copy(flare16x_canvas* source_canvas, uint16_t offset_x, uint16_t offset_y,
                                    uint16_t width, uint16_t height, flare16x_canvas* target_canvas)
{
    // Make sure that neither canvas is null (this requires short circuiting to work)
    if (source_canvas == NULL || source_canvas->pixels == NULL || target_canvas == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_CANVAS);

    // Verify the input rectangle
    if (width == 0 || height == 0 || source_canvas->width == 0 || source_canvas->height == 0 ||
        offset_x + width > source_canvas->width || offset_y + height > source_canvas->height)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_CANVAS);

    // Copy the width and the height
    target_canvas->width = width;
    target_canvas->height = height;

    // Allocate the target rectangle
    target_canvas->pixels = malloc(width * height * sizeof(uint16_t));
    if (target_canvas->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_CANVAS);

    // Finally, copy the pixels
    int x, y;
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            target_canvas->pixels[y * width + x] =
                    source_canvas->pixels[(y + offset_y) * source_canvas->width + x + offset_x];

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_CANVAS);
}

// Copies a canvas region to an offset on another canvas
flare16x_error flare16x_canvas_merge(flare16x_canvas* source_canvas,
                                     int16_t source_offset_x, int16_t source_offset_y,
                                     int16_t target_offset_x, int16_t target_offset_y,
                                     uint16_t width, uint16_t height, flare16x_canvas* target_canvas)
{
    // Make sure that neither canvas nor their pixels are null (this requires short circuiting to work)
    if (source_canvas == NULL || source_canvas->pixels == NULL || target_canvas == NULL ||
        target_canvas->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_CANVAS);

    // Check, if the width and height are valid
    if (width == 0 || height == 0)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_CANVAS);

    // Now, copy the pixels, while making sure that the point is within source and target bounds
    int x, y;
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            if (x + source_offset_x >= 0 && y + source_offset_y >= 0 &&
                x + target_offset_x >= 0 && y + target_offset_y >= 0 &&
                x + source_offset_x < source_canvas->width && y + source_offset_y < source_canvas->height &&
                x + target_offset_x < target_canvas->width && y + target_offset_y < target_canvas->height)
                target_canvas->pixels[(y + target_offset_y) * target_canvas->width + x + target_offset_x] =
                        source_canvas->pixels[(y + source_offset_y) * source_canvas->width + x + source_offset_x];

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_CANVAS);
}

// Destroys the canvas and frees its resources
flare16x_error flare16x_canvas_destroy(flare16x_canvas* canvas)
{
    // Make sure the canvas is not null
    if (canvas == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_CANVAS);

    // Free the pixel buffer
    free(canvas->pixels);
    // Clear the struct
    memset(canvas, 0, sizeof(flare16x_canvas));

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_CANVAS);
}

// Returns the specified pixel value via the pixel pointer
flare16x_error flare16x_canvas_get(uint16_t x, uint16_t y, flare16x_canvas* canvas, uint16_t* pixel)
{
    // Make sure the canvas and pixel pointer are not null
    if (canvas == NULL || pixel == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_CANVAS);

    // Verify the pixel boundaries
    if (canvas->width == 0 || canvas->height == 0 || x >= canvas->width || y >= canvas->height)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_CANVAS);

    // Read the pixel
    *pixel = canvas->pixels[y * canvas->width + x];

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_CANVAS);
}

// Attempts to set a pixel on the canvas to the supplied pixel value
flare16x_error flare16x_canvas_set(uint16_t x, uint16_t y, uint16_t pixel, flare16x_canvas* canvas)
{
    // Make sure the canvas is not null
    if (canvas == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_CANVAS);

    // Verify the pixel boundaries
    if (canvas->width == 0 || canvas->height == 0 || x >= canvas->width || y >= canvas->height)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_CANVAS);

    // Write the pixel
    canvas->pixels[y * canvas->width + x] = pixel;

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_CANVAS);
}
