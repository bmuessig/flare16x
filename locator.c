//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// locator.c: Functions for retrieving and identifying infrared image data
//

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "canvas.h"
#include "bitmap.h"

#include "locator.h"

// Cuts the input image into the IR image and text and initializes the locator
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_locator_create(flare16x_bitmap* screenshot, flare16x_locator* locator)
{
    // Make sure the screenshot and locator are not null
    if (screenshot == NULL || locator == NULL || screenshot->dib == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_LOCATOR);

    // Check, if the size of the bitmap matches (the rest of the validation is done by the bitmap edit function)
    if (screenshot->dib->width != FLARE16X_LOCATOR_EXPECTED_WIDTH ||
        abs(screenshot->dib->height) != FLARE16X_LOCATOR_EXPECTED_HEIGHT)
        return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_LOCATOR);

    // Zero the locator struct
    memset(locator, 0, sizeof(flare16x_locator));

    // Attempt to create a canvas from the full screenshot
    flare16x_canvas full_canvas;
    flare16x_error error;
    error = flare16x_bitmap_edit(screenshot, 0, 0, FLARE16X_LOCATOR_EXPECTED_WIDTH,
            FLARE16X_LOCATOR_EXPECTED_HEIGHT, &full_canvas);
    if (flare16x_error_reason(error) != FLARE16X_ERROR_NONE)
        return flare16x_error_wrap(flare16x_error_make(FLARE16X_ERROR_CALLEE, FLARE16X_ERROR_SOURCE_LOCATOR),
                error);

    // Allocate storage for the two canvas structs
    locator->text_canvas = malloc(sizeof(flare16x_canvas));
    if (locator->text_canvas == NULL)
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_LOCATOR);
    locator->ir_canvas = malloc(sizeof(flare16x_canvas));
    if (locator->ir_canvas == NULL)
    {
        free(locator->text_canvas);
        return flare16x_error_make(FLARE16X_ERROR_MALLOC, FLARE16X_ERROR_SOURCE_LOCATOR);
    }

    // Copy the text region
    error = flare16x_canvas_copy(&full_canvas, FLARE16X_LOCATOR_TEXT_OFFSET_X, FLARE16X_LOCATOR_TEXT_OFFSET_Y,
            FLARE16X_LOCATOR_TEXT_WIDTH, FLARE16X_LOCATOR_TEXT_HEIGHT, locator->text_canvas);
    if (flare16x_error_reason(error) != FLARE16X_ERROR_NONE)
        return flare16x_error_wrap(flare16x_error_make(FLARE16X_ERROR_CALLEE, FLARE16X_ERROR_SOURCE_LOCATOR),
                                   error);
    // And copy the IR region
    error = flare16x_canvas_copy(&full_canvas, FLARE16X_LOCATOR_IR_OFFSET_X, FLARE16X_LOCATOR_IR_OFFSET_Y,
                                 FLARE16X_LOCATOR_IR_WIDTH, FLARE16X_LOCATOR_IR_HEIGHT, locator->ir_canvas);
    if (flare16x_error_reason(error) != FLARE16X_ERROR_NONE)
        return flare16x_error_wrap(flare16x_error_make(FLARE16X_ERROR_CALLEE, FLARE16X_ERROR_SOURCE_LOCATOR),
                                   error);

    // Finally, free the full screenshot
    flare16x_canvas_destroy(&full_canvas);

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_LOCATOR);
}

// Attempts to identify the model of the device and locate the crosshair
flare16x_error flare16x_locator_process(flare16x_locator* locator)
{
    // Make sure the locator and its pointers are not null
    if (locator == NULL || locator->text_canvas == NULL || locator->ir_canvas == NULL ||
        locator->text_canvas->pixels == NULL || locator->ir_canvas == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_LOCATOR);

    // Verify the widths and heights
    if (locator->text_canvas->width != FLARE16X_LOCATOR_TEXT_WIDTH ||
        locator->text_canvas->height != FLARE16X_LOCATOR_TEXT_HEIGHT ||
        locator->ir_canvas->width != FLARE16X_LOCATOR_IR_WIDTH ||
        locator->ir_canvas->height != FLARE16X_LOCATOR_IR_HEIGHT)
        return flare16x_error_make(FLARE16X_ERROR_RANGE, FLARE16X_ERROR_SOURCE_LOCATOR);

    // Fetch the minimum expected border and fill
    int expected_border = FLARE16X_LOCATOR_CROSSHAIR_BORDER_WIDTH,
        expected_fill = FLARE16X_LOCATOR_TG165_FILL_WIDTH;
    if (FLARE16X_LOCATOR_TG167_FILL_WIDTH < expected_fill)
        expected_fill = FLARE16X_LOCATOR_TG167_FILL_WIDTH;
    // Duplicate the expected fill, since there are two fill regions in the search line
    expected_fill *= 2;

    // Scan through the image line by line and count the border and fill pixels
    int x, y, actual_border = 0, actual_fill = 0, actual_eye = 0;
    for (y = 0; y < locator->ir_canvas->height; y++, actual_border = 0, actual_fill = 0)
        for (x = 0; x < locator->ir_canvas->width; x++)
        {
            // Fetch the pixel
            uint16_t pixel = flare16x_canvas_raw(x, y, locator->ir_canvas);

            // Check the pixel and increment the correct counters
            if (pixel == FLARE16X_LOCATOR_CROSSHAIR_BORDER)
                actual_border++;
            else if (pixel == FLARE16X_LOCATOR_CROSSHAIR_FILL)
                actual_fill++;

            // Check, if the line reaches or exceeds the criteria
            if (actual_border >= expected_border && actual_fill >= expected_fill)
            {
                // Stop scanning this line and reset the counters
                actual_border = 0, actual_fill = 0, actual_eye = 0;

                // Keep track of the state
                int state = FLARE16X_LOCATOR_STATE_START;

                // Start scanning the line again and search for the pattern
                for (x = 0; x < locator->ir_canvas->width; x++)
                {
                    // First, the pixel has to be fetched
                    pixel = flare16x_canvas_raw(x, y, locator->ir_canvas);

                    switch (pixel)
                    {
                        case FLARE16X_LOCATOR_CROSSHAIR_BORDER:
                            // For border pixels, check the state
                            if (state == FLARE16X_LOCATOR_STATE_FILL_1 && actual_border == 1 &&
                              (actual_fill == FLARE16X_LOCATOR_TG165_FILL_WIDTH ||
                               actual_fill == FLARE16X_LOCATOR_TG167_FILL_WIDTH))
                            {
                                // FILL_1 -> BORDER_2
                                state = FLARE16X_LOCATOR_STATE_BORDER_2;
                                actual_border++;
                            } else if (state == FLARE16X_LOCATOR_STATE_EYE && actual_border == 2 &&
                                        (actual_eye == FLARE16X_LOCATOR_TG165_CENTER_WIDTH ||
                                        actual_eye == FLARE16X_LOCATOR_TG167_CENTER_WIDTH))
                            {
                                // EYE -> BORDER_3
                                state = FLARE16X_LOCATOR_STATE_BORDER_3;
                                actual_border++;
                            } else if (state == FLARE16X_LOCATOR_STATE_FILL_2 && actual_border == 3 &&
                                     (actual_fill == FLARE16X_LOCATOR_TG165_FILL_WIDTH * 2 ||
                                      actual_fill == FLARE16X_LOCATOR_TG167_FILL_WIDTH * 2))
                            {
                                // FILL_2 -> BORDER_4
                                state = FLARE16X_LOCATOR_STATE_BORDER_4;
                                actual_border++;
                            } else
                            {
                                // START or any other reset condition -> BORDER_1
                                state = FLARE16X_LOCATOR_STATE_BORDER_1;
                                actual_border = 1, actual_fill = 0, actual_eye = 0;
                            }
                            break;
                        case FLARE16X_LOCATOR_CROSSHAIR_FILL:
                            // For fill pixels, check the state
                            if (state == FLARE16X_LOCATOR_STATE_BORDER_1 && actual_border == 1)
                            {
                                // BORDER_1 -> FILL_1
                                state = FLARE16X_LOCATOR_STATE_FILL_1;
                                actual_fill++;
                            } else if (state == FLARE16X_LOCATOR_STATE_BORDER_3 && actual_border == 3)
                            {
                                // BORDER_3 -> FILL_2
                                state = FLARE16X_LOCATOR_STATE_FILL_2;
                                actual_fill++;
                            } else if (state == FLARE16X_LOCATOR_STATE_FILL_1 ||
                                        state == FLARE16X_LOCATOR_STATE_FILL_2)
                                actual_fill++;
                            else
                            {
                                // Any reset condition -> START
                                state = FLARE16X_LOCATOR_STATE_START;
                                actual_border = 0, actual_fill = 0, actual_eye = 0;
                            }
                            break;
                        default:
                            // For other pixels, check the state
                            if (state == FLARE16X_LOCATOR_STATE_BORDER_2 && actual_border == 2)
                            {
                                // BORDER_2 -> EYE
                                state = FLARE16X_LOCATOR_STATE_EYE;
                                actual_eye++;
                            } else if (state == FLARE16X_LOCATOR_STATE_EYE)
                                actual_eye++;
                            else
                            {
                                // Any reset condition -> START
                                state = FLARE16X_LOCATOR_STATE_START;
                                actual_border = 0, actual_fill = 0, actual_eye = 0;
                            }
                            break;
                    }

                    // Check, if the border is finished
                    if (actual_border != FLARE16X_LOCATOR_CROSSHAIR_BORDER_WIDTH)
                        continue;

                    // Now, try to determine a device model
                    if (actual_fill == FLARE16X_LOCATOR_TG165_FILL_WIDTH * 2 &&
                        actual_eye == FLARE16X_LOCATOR_TG165_CENTER_WIDTH)
                    {
                        // The device is a TG165
                        locator->device_model = FLARE16X_LOCATOR_MODEL_TG165;

                        // Set up the dimensions of the aperture and crosshair
                        locator->aperture_height = FLARE16X_LOCATOR_TG165_CENTER_HEIGHT;
                        locator->aperture_width = FLARE16X_LOCATOR_TG165_CENTER_WIDTH;
                        locator->crosshair_height = FLARE16X_LOCATOR_TG165_CROSSHAIR_HEIGHT;
                        locator->crosshair_width = FLARE16X_LOCATOR_CROSSHAIR_BORDER_WIDTH +
                                FLARE16X_LOCATOR_TG165_CENTER_WIDTH + FLARE16X_LOCATOR_TG165_FILL_WIDTH * 2;

                        // Calculate the positions of the crosshair and aperture
                        locator->crosshair_x = x + 1 - locator->crosshair_width;
                        locator->crosshair_y = y - FLARE16X_LOCATOR_TG165_TARGET_ROW;
                        locator->aperture_x = locator->crosshair_x + FLARE16X_LOCATOR_TG165_CENTER_OFFSET_X;
                        locator->aperture_y = locator->crosshair_y + FLARE16X_LOCATOR_TG165_CENTER_OFFSET_Y;

                    } else if (actual_fill == FLARE16X_LOCATOR_TG167_FILL_WIDTH * 2 &&
                            actual_eye == FLARE16X_LOCATOR_TG167_CENTER_WIDTH)
                    {
                        // The device is a TG167
                        locator->device_model = FLARE16X_LOCATOR_MODEL_TG167;

                        // Set up the dimensions of the aperture and crosshair
                        locator->aperture_height = FLARE16X_LOCATOR_TG167_CENTER_HEIGHT;
                        locator->aperture_width = FLARE16X_LOCATOR_TG167_CENTER_WIDTH;
                        locator->crosshair_height = FLARE16X_LOCATOR_TG167_CROSSHAIR_HEIGHT;
                        locator->crosshair_width = FLARE16X_LOCATOR_CROSSHAIR_BORDER_WIDTH +
                                FLARE16X_LOCATOR_TG167_CENTER_WIDTH + FLARE16X_LOCATOR_TG167_FILL_WIDTH * 2;

                        // Calculate the positions of the crosshair and aperture
                        locator->crosshair_x = x + 1 - locator->crosshair_width;
                        locator->crosshair_y = y - FLARE16X_LOCATOR_TG167_TARGET_ROW;
                        locator->aperture_x = locator->crosshair_x + FLARE16X_LOCATOR_TG167_CENTER_OFFSET_X;
                        locator->aperture_y = locator->crosshair_y + FLARE16X_LOCATOR_TG167_CENTER_OFFSET_Y;

                    } else
                        continue;

                    // And return success
                    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_LOCATOR);
                }

                // If the line does not contain the desired pattern, jump to the next one
                break;
            }
        }

    // No pattern was found, but the image is still valid
    locator->device_model = FLARE16X_LOCATOR_MODEL_UNKNOWN;
    return flare16x_error_make(FLARE16X_ERROR_IMAGE, FLARE16X_ERROR_SOURCE_LOCATOR);
}

// Detects the crosshair state of a particular pixel
// This function yields optimization potential, as it could be changed to be mask based
uint8_t flare16x_locator_detect(flare16x_locator* locator, uint16_t x, uint16_t y)
{
    // Check, if the locator is valid
    if (locator == NULL || locator->ir_canvas == NULL || locator->ir_canvas->width < 1 ||
        locator->ir_canvas->height < 1)
        return FLARE16X_LOCATOR_DETECT_FAIL;

    // Make sure the pixel is within bounds
    if (x >= locator->ir_canvas->width || y >= locator->ir_canvas->height)
        return FLARE16X_LOCATOR_DETECT_BOUNDS;

    // Check the model
    switch (locator->device_model)
    {
        case FLARE16X_LOCATOR_MODEL_TG165:
            // Make sure the crosshair is valid
            if (locator->crosshair_height != FLARE16X_LOCATOR_TG165_CROSSHAIR_HEIGHT ||
                locator->crosshair_width != FLARE16X_LOCATOR_CROSSHAIR_BORDER_WIDTH +
                FLARE16X_LOCATOR_TG165_CENTER_WIDTH + FLARE16X_LOCATOR_TG165_FILL_WIDTH * 2)
                return FLARE16X_LOCATOR_DETECT_FAIL;

            // Check, if the pixel is outside the crosshair region first to preserve CPU cycles
            if (!flare16x_locator_is_within(x, y, locator->crosshair_x, locator->crosshair_y,
                                            locator->crosshair_width, locator->crosshair_height))
                return FLARE16X_LOCATOR_DETECT_IMAGE;

            // Check the pixels
            // x,y,w,h
            // 6,6,11,3
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 6, locator->crosshair_y + 6,
                                           11, 3))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 0,10,6,3
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 0, locator->crosshair_y + 10,
                                           6, 3))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 17,10,6,3
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 17, locator->crosshair_y + 10,
                                           6, 3))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 10,17,3,6
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 10, locator->crosshair_y + 17,
                                           3, 6))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 6,9,3,8
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 6, locator->crosshair_y + 9,
                                           3, 8))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 14,9,3,8
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 14, locator->crosshair_y + 9,
                                           3, 8))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 10,0,3,6
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 10, locator->crosshair_y + 0,
                                           3, 6))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 9,14,5,3
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 9, locator->crosshair_y + 14,
                                           5, 3))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;

            // If no match occurred, it's a regular part of the image data
            return FLARE16X_LOCATOR_DETECT_IMAGE;

        case FLARE16X_LOCATOR_MODEL_TG167:
            // Make sure the crosshair is valid
            if (locator->crosshair_height != FLARE16X_LOCATOR_TG167_CROSSHAIR_HEIGHT ||
                locator->crosshair_width != FLARE16X_LOCATOR_CROSSHAIR_BORDER_WIDTH +
                                            FLARE16X_LOCATOR_TG167_CENTER_WIDTH + FLARE16X_LOCATOR_TG167_FILL_WIDTH * 2)
                return FLARE16X_LOCATOR_DETECT_FAIL;

            // Check, if the pixel is outside the crosshair region first to preserve CPU cycles
            if (!flare16x_locator_is_within(x, y, locator->crosshair_x, locator->crosshair_y,
                                            locator->crosshair_width, locator->crosshair_height))
                return FLARE16X_LOCATOR_DETECT_IMAGE;

            // Check the pixels
            // x,y,w,h
            // 13,12,23,3
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 13, locator->crosshair_y + 12,
                                           23, 3))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 13,32,23,3
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 13, locator->crosshair_y + 32,
                                           23, 3))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 0,22,13,3
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 0, locator->crosshair_y + 22,
                                           13, 3))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 36,22,13,3
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 36, locator->crosshair_y + 22,
                                           13, 3))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 23,35,3,12
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 23, locator->crosshair_y + 35,
                                           3, 12))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 13,15,3,17
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 13, locator->crosshair_y + 15,
                                           3, 17))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 33,15,3,17
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 33, locator->crosshair_y + 15,
                                           3, 17))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;
            // 23,0,3,12
            if (flare16x_locator_is_within(x, y, locator->crosshair_x + 23, locator->crosshair_y + 0,
                                           3, 12))
                return FLARE16X_LOCATOR_DETECT_CROSSHAIR;

            // If no match occurred, it's a regular part of the image data
            return FLARE16X_LOCATOR_DETECT_IMAGE;

        case FLARE16X_LOCATOR_MODEL_UNKNOWN:
            // For an unknown model, it is assumed that the entire canvas is actual IR data
            return FLARE16X_LOCATOR_DETECT_IMAGE;

        default:
            return FLARE16X_LOCATOR_DETECT_FAIL;
    }
}

// Frees all resources used by a locator struct
flare16x_error flare16x_locator_destroy(flare16x_locator* locator)
{
    // Make sure the locator is not null
    if (locator == NULL)
        return flare16x_error_make(FLARE16X_ERROR_NULL, FLARE16X_ERROR_SOURCE_LOCATOR);

    // Free the resources by destroying the two canvas structs first
    flare16x_canvas_destroy(locator->text_canvas);
    flare16x_canvas_destroy(locator->ir_canvas);

    // Followed by destroying the structs themselves
    free(locator->text_canvas);
    free(locator->ir_canvas);

    // Finally, zero the struct
    memset(locator, 0, sizeof(flare16x_locator));

    return flare16x_error_make(FLARE16X_ERROR_NONE, FLARE16X_ERROR_SOURCE_LOCATOR);
}
