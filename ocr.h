//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// ocr.h: Header file for the optical character recognition functions for the OSD text
//

#ifndef FLARE16X_OCR_H
#define FLARE16X_OCR_H

#include "error.h"
#include "canvas.h"

#define FLARE16X_OCR_LARGE_WIDTH 18
#define FLARE16X_OCR_LARGE_HEIGHT 23
#define FLARE16X_OCR_LARGE_COLOR flare16x_canvas_rgb888(0xff, 0xff, 0xff)

#define FLARE16X_OCR_SMALL_WIDTH 10
#define FLARE16X_OCR_SMALL_HEIGHT 12
#define FLARE16X_OCR_SMALL_COLOR flare16x_canvas_rgb888(0xff, 0xff, 0xff)

// Attempts to detect a single large font character
flare16x_error flare16x_ocr_large_char(uint16_t offset_x, uint16_t offset_y, flare16x_canvas* canvas,
                                       char* result_char);

// Attempts to detect a large string (with capacity length + 1) of length characters
flare16x_error flare16x_ocr_large_string(uint16_t offset_x, uint16_t offset_y, uint16_t pitch, uint16_t length,
                                         uint16_t max_unknown, flare16x_canvas* canvas, char* result_string);

// Attempts to detect a single small font character
flare16x_error flare16x_ocr_small_char(uint16_t offset_x, uint16_t offset_y, flare16x_canvas* canvas,
                                       char* result_char);

// Attempts to detect a small string (with capacity length + 1) of length characters
flare16x_error flare16x_ocr_small_string(uint16_t offset_x, uint16_t offset_y, uint16_t pitch, uint16_t length,
                                         uint16_t max_unknown, flare16x_canvas* canvas, char* result_string);

#endif //FLARE16X_OCR_H
