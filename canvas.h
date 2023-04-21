//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// canvas.h: Header file for canvas image manipulation functions
//

#ifndef FLARE16X_CANVAS_H
#define FLARE16X_CANVAS_H

#include <stdint.h>

#include "error.h"

// RGB canvas buffer
typedef struct {
    // The width of the buffer
    uint16_t width;
    // The height of the buffer
    uint16_t height;
    // The RGB565 pixel buffer
    uint16_t* pixels;
} flare16x_canvas;

// Define a canvas color by its RGB565 component value
#define flare16x_canvas_rgb(r,g,b) ((uint16_t)((((r) & 0x1f) << 11) | (((g) & 0x3f) << 5) | ((b) & 0x1f)))
// Define a canvas color by its RGB888 component value
#define flare16x_canvas_rgb888(r,g,b) ((uint16_t)((((r) & 0xf8u) << 8) | (((g) & 0xfcu) << 3) | (((b) & 0xf8u) >> 3)))
// Raw access to a pixel (read and write)
#define flare16x_canvas_raw(x,y,canvas) (canvas)->pixels[(y) * (canvas)->width + (x)]

// Creates a new canvas and allocates the required memory
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_canvas_create(uint16_t width, uint16_t height, flare16x_canvas* canvas);

// Copies a canvas region and creates a new canvas from it
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_canvas_copy(flare16x_canvas* source_canvas, uint16_t offset_x, uint16_t offset_y,
                                    uint16_t width, uint16_t height, flare16x_canvas* target_canvas);

// Copies a canvas region to an offset on another canvas
flare16x_error flare16x_canvas_merge(flare16x_canvas* source_canvas,
                                        int16_t source_offset_x, int16_t source_offset_y,
                                        int16_t target_offset_x, int16_t target_offset_y,
                                        uint16_t width, uint16_t height, flare16x_canvas* target_canvas);

// Destroys the canvas and frees its resources
flare16x_error flare16x_canvas_destroy(flare16x_canvas* canvas);

// Returns the specified pixel value via the pixel pointer
flare16x_error flare16x_canvas_get(uint16_t x, uint16_t y, flare16x_canvas* canvas, uint16_t* pixel);

// Attempts to set a pixel on the canvas to the supplied pixel value
flare16x_error flare16x_canvas_set(uint16_t x, uint16_t y, uint16_t pixel, flare16x_canvas* canvas);

#endif //FLARE16X_CANVAS_H
