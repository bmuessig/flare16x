//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// ocr.c: Optical character recognition functions for the OSD text
//

#include <stdint.h>
#include <string.h>

#include "error.h"
#include "canvas.h"

#include "ocr.h"

// Attempts to detect a single large font character
flare16x_error flare16x_ocr_large_char(uint16_t offset_x, uint16_t offset_y, flare16x_canvas* canvas,
                                        char* result_char)
{
    // Verify that the canvas and output char are not null pointers
    if (canvas == NULL || result_char == NULL || canvas->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_OCR);

    // Check, that the width and height are valid
    if (canvas->width == 0 || canvas->height == 0)
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_OCR);

    // Also check that the offsets, width and height are correct
    if (offset_x + FLARE16X_OCR_LARGE_WIDTH > canvas->width || offset_y + FLARE16X_OCR_LARGE_HEIGHT > canvas->height)
        return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_OCR);

    // Next, calculate the signature
    uint8_t signature = 0;
    uint16_t search_color = FLARE16X_OCR_LARGE_COLOR;

    if (flare16x_canvas_raw(offset_x + 10, offset_y + 1, canvas) == search_color)
        signature |= 1 << 0;
    if (flare16x_canvas_raw(offset_x + 16, offset_y + 1, canvas) == search_color)
        signature |= 1 << 1;
    if (flare16x_canvas_raw(offset_x + 3, offset_y + 4, canvas) == search_color)
        signature |= 1 << 2;
    if (flare16x_canvas_raw(offset_x + 15, offset_y + 4, canvas) == search_color)
        signature |= 1 << 3;
    if (flare16x_canvas_raw(offset_x + 12, offset_y + 7, canvas) == search_color)
        signature |= 1 << 4;
    if (flare16x_canvas_raw(offset_x + 8, offset_y + 11, canvas) == search_color)
        signature |= 1 << 5;
    if (flare16x_canvas_raw(offset_x + 16, offset_y + 14, canvas) == search_color)
        signature |= 1 << 6;
    if (flare16x_canvas_raw(offset_x + 8, offset_y + 18, canvas) == search_color)
        signature |= 1 << 7;

    // Finally, match the signature against known values
    switch (signature)
    {
        case 0x41:
            *result_char = '0';
            break;
        case 0x11:
            *result_char = '1';
            break;
        case 0x8d:
            *result_char = '2';
            break;
        case 0x35:
            *result_char = '3';
            break;
        case 0x51:
            *result_char = '4';
            break;
        case 0x01:
            *result_char = '5';
            break;
        case 0x69:
            *result_char = '6';
            break;
        case 0xbb:
            *result_char = '7';
            break;
        case 0x7d:
            *result_char = '8';
            break;
        case 0x25:
            *result_char = '9';
            break;
        case 0x00:
            *result_char = ' ';
            break;
        case 0x28:
            *result_char = 'C';
            break;
        case 0x30:
            *result_char = 'F';
            break;
        case 0x80:
            *result_char = '.';
            break;
        case 0x84:
            *result_char = 'L';
            break;
        case 0x20:
            *result_char = '-';
            break;
        case 0xcc:
            *result_char = 'O';
            break;
        default:
            *result_char = 0;
            return flare16x_error_make(FLARE16X_ERROR_UNKNOWN, FLARE16X_ERROR_SOURCE_OCR);
    }

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_OCR);
}

// Attempts to detect a large string (with capacity length + 1) of length characters
flare16x_error flare16x_ocr_large_string(uint16_t offset_x, uint16_t offset_y, uint16_t pitch, uint16_t length,
        uint16_t max_unknown, flare16x_canvas* canvas, char* result_string)
{
    // Verify that the canvas and output char are not null pointers
    if (canvas == NULL || result_string == NULL || canvas->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_OCR);

    // Check, that the width and height are valid
    if (canvas->width == 0 || canvas->height == 0)
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_OCR);

    // Also check that the offsets, width and height are correct
    if (length == 0 || (FLARE16X_OCR_LARGE_WIDTH + pitch) * length + offset_x > canvas->width + pitch ||
        offset_y + FLARE16X_OCR_LARGE_HEIGHT > canvas->height)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_OCR);

    // Now, clear the output string
    memset(result_string, 0, length + 1);

    // Next, loop through the characters and end on the first error
    int src_digit, dst_digit;
    for (src_digit = 0, dst_digit = 0; src_digit < length; src_digit++)
    {
        flare16x_error error;
        error = flare16x_ocr_large_char((FLARE16X_OCR_LARGE_WIDTH + pitch) * src_digit + offset_x, offset_y,
                canvas, result_string + dst_digit);
        switch (flare16x_error_reason(error))
        {
            case FLARE16X_ERROR_NONE:
                // No error, move on to the next digit
                dst_digit++;
                break;

            case FLARE16X_ERROR_UNKNOWN:
                // Unknown digit, don't output it
                // If the maximum number of unknown digits is reached, throw an error
                if (max_unknown == 0)
                    return error;

                // Otherwise, decrement the remaining counter of unknown values and continue
                max_unknown--;
                continue;

            default:
                return error;
        }
    }

    // Fall through to success
    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_OCR);
}

// Attempts to detect a single small font character
flare16x_error flare16x_ocr_small_char(uint16_t offset_x, uint16_t offset_y, flare16x_canvas* canvas,
                                       char* result_char)
{
    // Verify that the canvas and output char are not null pointers
    if (canvas == NULL || result_char == NULL || canvas->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_OCR);

    // Check, that the width and height are valid
    if (canvas->width == 0 || canvas->height == 0)
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_OCR);

    // Also check that the offsets, width and height are correct
    if (offset_x + FLARE16X_OCR_SMALL_WIDTH > canvas->width || offset_y + FLARE16X_OCR_SMALL_HEIGHT > canvas->height)
        return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_OCR);

    // Next, calculate the signature
    uint8_t signature = 0;
    uint16_t search_color = FLARE16X_OCR_SMALL_COLOR;

    if (flare16x_canvas_raw(offset_x + 3, offset_y + 1, canvas) == search_color)
        signature |= 1 << 0;
    if (flare16x_canvas_raw(offset_x + 5, offset_y + 2, canvas) == search_color)
        signature |= 1 << 1;
    if (flare16x_canvas_raw(offset_x + 1, offset_y + 4, canvas) == search_color)
        signature |= 1 << 2;
    if (flare16x_canvas_raw(offset_x + 6, offset_y + 5, canvas) == search_color)
        signature |= 1 << 3;
    if (flare16x_canvas_raw(offset_x + 4, offset_y + 8, canvas) == search_color)
        signature |= 1 << 4;
    if (flare16x_canvas_raw(offset_x + 7, offset_y + 8, canvas) == search_color)
        signature |= 1 << 5;
    if (flare16x_canvas_raw(offset_x + 5, offset_y + 10, canvas) == search_color)
        signature |= 1 << 6;
    if (flare16x_canvas_raw(offset_x + 7, offset_y + 10, canvas) == search_color)
        signature |= 1 << 7;

    // Finally, match the signature against known values
    switch (signature)
    {
        case 0x25:
            *result_char = '0';
            break;
        case 0x52:
            *result_char = '1';
            break;
        case 0xd0:
            *result_char = '2';
            break;
        case 0x89:
            *result_char = '3';
            break;
        case 0xb2:
            *result_char = '4';
            break;
        case 0x29:
            *result_char = '5';
            break;
        case 0x6d:
            *result_char = '6';
            break;
        case 0x19:
            *result_char = '7';
            break;
        case 0x21:
            *result_char = '8';
            break;
        case 0xc0:
            *result_char = '9';
            break;
        case 0x00:
            *result_char = ' ';
            break;
        case 0x40:
            *result_char = '.';
            break;
        case 0x12:
            *result_char = ':';
            break;
        case 0xc9:
            *result_char = 'E';
            break;
        default:
            *result_char = 0;
            return flare16x_error_make(FLARE16X_ERROR_UNKNOWN, FLARE16X_ERROR_SOURCE_OCR);
    }

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_OCR);
}

// Attempts to detect a small string (with capacity length + 1) of length characters
flare16x_error flare16x_ocr_small_string(uint16_t offset_x, uint16_t offset_y, uint16_t pitch, uint16_t length,
                                         uint16_t max_unknown, flare16x_canvas* canvas, char* result_string)
{
    // Verify that the canvas and output char are not null pointers
    if (canvas == NULL || result_string == NULL || canvas->pixels == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_OCR);

    // Check, that the width and height are valid
    if (canvas->width == 0 || canvas->height == 0)
        return flare16x_error_make(FLARE16X_ERROR_FORMAT, FLARE16X_ERROR_SOURCE_OCR);

    // Also check that the offsets, width and height are correct
    if (length == 0 || (FLARE16X_OCR_SMALL_WIDTH + pitch) * length + offset_x > canvas->width + pitch ||
        offset_y + FLARE16X_OCR_SMALL_HEIGHT > canvas->height)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_OCR);

    // Now, clear the output string
    memset(result_string, 0, length + 1);

    // Next, loop through the characters and end on the first error
    int src_digit, dst_digit;
    for (src_digit = 0, dst_digit = 0; src_digit < length; src_digit++)
    {
        flare16x_error error;
        error = flare16x_ocr_small_char((FLARE16X_OCR_SMALL_WIDTH + pitch) * src_digit + offset_x, offset_y,
                                        canvas, result_string + dst_digit);
        switch (flare16x_error_reason(error))
        {
            case FLARE16X_ERROR_NONE:
                // No error, move on to the next digit
                dst_digit++;
                break;

            case FLARE16X_ERROR_UNKNOWN:
                // Unknown digit, don't output it
                // If the maximum number of unknown digits is reached, throw an error
                if (max_unknown == 0)
                    return error;

                // Otherwise, decrement the remaining counter of unknown values and continue
                max_unknown--;
                continue;

            default:
                return error;
        }
    }

    // Fall through to success
    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_OCR);
}
