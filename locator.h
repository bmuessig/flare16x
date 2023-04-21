//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// locator.c: Header file for retrieving and identifying infrared image data
//

#ifndef FLARE16X_LOCATOR_H
#define FLARE16X_LOCATOR_H

#include <stdint.h>

#include "error.h"
#include "canvas.h"
#include "bitmap.h"

// Device model enum
enum {
    // The scan has not been run yet
    FLARE16X_LOCATOR_MODEL_TBD,
    // The model could not be determined
    FLARE16X_LOCATOR_MODEL_UNKNOWN,
    // TG165
    FLARE16X_LOCATOR_MODEL_TG165,
    // TG167
    FLARE16X_LOCATOR_MODEL_TG167
};

// Detection state enum
enum {
    FLARE16X_LOCATOR_STATE_START,
    FLARE16X_LOCATOR_STATE_BORDER_1,
    FLARE16X_LOCATOR_STATE_FILL_1,
    FLARE16X_LOCATOR_STATE_BORDER_2,
    FLARE16X_LOCATOR_STATE_EYE,
    FLARE16X_LOCATOR_STATE_BORDER_3,
    FLARE16X_LOCATOR_STATE_FILL_2,
    FLARE16X_LOCATOR_STATE_BORDER_4
};

// Crosshair detection state enum
enum {
    // The query failed due to an error
    FLARE16X_LOCATOR_DETECT_FAIL,
    // The point specified is out of bounds
    FLARE16X_LOCATOR_DETECT_BOUNDS,
    // The point specified is part of valid image data
    FLARE16X_LOCATOR_DETECT_IMAGE,
    // The point specified is part of the crosshair
    FLARE16X_LOCATOR_DETECT_CROSSHAIR,
    // The point specified has an invalid color
    FLARE16X_LOCATOR_DETECT_INVALID
};

/*
 * Theory of operation
 * 1) Scan the IR image fragment line by line and count the number of border and fill pixels
 * 2) If the count of both values is greater than or equal to the expected reference value, goto 3, otherwise continue
 * 3) Now, do an incremental, regex-like search on the candidate line, incrementing the start position on every try
 *    It is important to have the correct order and sequence of border and fill pixels, ignoring the eye
 *    All models are checked in both the quick pre-scan (search by smallest count of all models)
 * 4) If there was an exact match, stop the search and calculate the crosshair rectangle
 */

// Expected screenshot width and height
// The expected width of the full screenshot
#define FLARE16X_LOCATOR_EXPECTED_WIDTH 174
// The expected height of the full screenshot
#define FLARE16X_LOCATOR_EXPECTED_HEIGHT 220

// Text window
// The x-offset of the text area
#define FLARE16X_LOCATOR_TEXT_OFFSET_X 2
// The y-offset of the text area
#define FLARE16X_LOCATOR_TEXT_OFFSET_Y 1
// The width of the text area
#define FLARE16X_LOCATOR_TEXT_WIDTH 170
// The height of the text area
#define FLARE16X_LOCATOR_TEXT_HEIGHT 23

// Temperature window
// The x-offset of the temperature relative to the text window
#define FLARE16X_LOCATOR_TEMPERATURE_OFFSET_X 0
// The y-offset of the temperature relative to the text window
#define FLARE16X_LOCATOR_TEMPERATURE_OFFSET_Y 0
// The number of temperature digits
#define FLARE16X_LOCATOR_TEMPERATURE_DIGITS 6
// The pitch of the temperature digits
#define FLARE16X_LOCATOR_TEMPERATURE_PITCH 0

// Emissivity window
// The x-offset of the emissivity relative to the text window
#define FLARE16X_LOCATOR_EMISSIVITY_OFFSET_X 110
// The y-offset of the emissivity relative to the text window
#define FLARE16X_LOCATOR_EMISSIVITY_OFFSET_Y 3
// The number of emissivity digits
#define FLARE16X_LOCATOR_EMISSIVITY_DIGITS 6
// The pitch of the emissivity digits
#define FLARE16X_LOCATOR_EMISSIVITY_PITCH 0

// IR window
// The x-offset of the IR area
#define FLARE16X_LOCATOR_IR_OFFSET_X 12
// The y-offset of the IR area
#define FLARE16X_LOCATOR_IR_OFFSET_Y 25
// The width of the IR area
#define FLARE16X_LOCATOR_IR_WIDTH 150
// The height of the IR area
#define FLARE16X_LOCATOR_IR_HEIGHT 175

// Crosshair
// The black border around the crosshair
#define FLARE16X_LOCATOR_CROSSHAIR_BORDER flare16x_canvas_rgb888(0x00, 0x00, 0x00)
// The white fill of the crosshair
#define FLARE16X_LOCATOR_CROSSHAIR_FILL flare16x_canvas_rgb888(0xff, 0xff, 0xff)
// Accumulated width of the border in all instances
#define FLARE16X_LOCATOR_CROSSHAIR_BORDER_WIDTH 4

// TG165
// Height of the entire crosshair
#define FLARE16X_LOCATOR_TG165_CROSSHAIR_HEIGHT 23
// Width of the fill on both sides
#define FLARE16X_LOCATOR_TG165_FILL_WIDTH 7
// Width of the center aperture
#define FLARE16X_LOCATOR_TG165_CENTER_WIDTH 5
// Height of the center aperture
#define FLARE16X_LOCATOR_TG165_CENTER_HEIGHT 5
// X-offset of the center aperture relative to the crosshair's origin
#define FLARE16X_LOCATOR_TG165_CENTER_OFFSET_X 9
// Y-offset of the center aperture relative to the crosshair's origin
#define FLARE16X_LOCATOR_TG165_CENTER_OFFSET_Y 9
// The target row relative to the height of the crosshair that is searched for
#define FLARE16X_LOCATOR_TG165_TARGET_ROW 11

// TG167
// Height of the entire crosshair
#define FLARE16X_LOCATOR_TG167_CROSSHAIR_HEIGHT 47
// Width of the fill on both sides
#define FLARE16X_LOCATOR_TG167_FILL_WIDTH 14
// Width of the center aperture
#define FLARE16X_LOCATOR_TG167_CENTER_WIDTH 17
// Height of the center aperture
#define FLARE16X_LOCATOR_TG167_CENTER_HEIGHT 17
// X-offset of the center aperture relative to the crosshair's origin
#define FLARE16X_LOCATOR_TG167_CENTER_OFFSET_X 16
// Y-offset of the center aperture relative to the crosshair's origin
#define FLARE16X_LOCATOR_TG167_CENTER_OFFSET_Y 15
// The target row relative to the height of the crosshair that is searched for
#define FLARE16X_LOCATOR_TG167_TARGET_ROW 23

// The locator struct holding the detected crosshair coordinates and image fragments
typedef struct {
    // The canvas containing the temperature and emissivity text
    flare16x_canvas* text_canvas;
    // The canvas containing the IR image
    flare16x_canvas* ir_canvas;
    // The x-coordinate of the crosshair's upper left origin relative to the IR canvas
    uint16_t crosshair_x;
    // The y-coordinate of the crosshair's upper left origin relative to the IR canvas
    uint16_t crosshair_y;
    // The width of the entire crosshair
    uint16_t crosshair_width;
    // The height of the entire crosshair
    uint16_t crosshair_height;
    // The x-coordinate of the aperture's upper left origin relative to the IR canvas
    uint16_t aperture_x;
    // The y-coordinate of the aperture's upper left origin relative to the IR canvas
    uint16_t aperture_y;
    // The width of the crosshair's aperture
    uint16_t aperture_width;
    // The height of the crosshair's aperture
    uint16_t aperture_height;
    // The detected model of the device
    uint8_t device_model;
} flare16x_locator;

// Verify that a coordinate is within a region of interest
#define flare16x_locator_is_within(x,y,roi_x,roi_y,roi_width,roi_height) \
((x) >= (roi_x) && (y) >= (roi_y) && (x) < (roi_x) + (roi_width) && (y) < (roi_y) + (roi_height))

// Cuts the input image into the IR image and text and initializes the locator
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_locator_create(flare16x_bitmap* screenshot, flare16x_locator* locator);

// Attempts to identify the model of the device and locate the crosshair
flare16x_error flare16x_locator_process(flare16x_locator* locator);

// Detects the crosshair state of a particular pixel
uint8_t flare16x_locator_detect(flare16x_locator* locator, uint16_t x, uint16_t y);

// Frees all resources used by a locator struct
flare16x_error flare16x_locator_destroy(flare16x_locator* locator);

#endif //FLARE16X_LOCATOR_H
