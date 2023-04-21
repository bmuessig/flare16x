//
// flare16x core
// Developed 2019 by Benedikt Muessig <github@bmuessig.eu>
// Licensed under GPLv3
//
// thermal.c: Header file for working with infrared image data
//

#ifndef FLARE16X_THERMAL_H
#define FLARE16X_THERMAL_H

#include <stdint.h>

#include "locator.h"
#include "canvas.h"

#define FLARE16X_THERMAL_MAX_POINTS (1 << 24)

// Make sure the thermal point struct aligns to 16-bit word boundaries
#pragma pack(push, 2)

// Represents a single thermal point
typedef struct {
    // The relative value of the thermal point
    uint8_t value;
    // The relative uncertainty (how many other values could it have been) in counts
    uint8_t uncertainty;
} flare16x_thermal_point;

// Enum describing the quantification mode used to get
enum {
    // For raw data input, exact values can be used but the accuracy will not be kept
    FLARE16X_THERMAL_QUANTIFICATION_EXACT,
    // For quantification, the highest thermal value for the matched color is assumed
    FLARE16X_THERMAL_QUANTIFICATION_FLOOR,
    // For quantification, the lowest thermal value for the matched color is assumed
    FLARE16X_THERMAL_QUANTIFICATION_CEILING,
    // For quantification, the highest median thermal value for the matched color is assumed
    FLARE16X_THERMAL_QUANTIFICATION_MEDIAN_HIGH,
    // For quantification, the lowest median thermal value for the matched color is assumed
    FLARE16X_THERMAL_QUANTIFICATION_MEDIAN_LOW,
    // The number of quantification mode enum values
    FLARE16X_THERMAL_QUANTIFICATION_COUNT
};

// Represents a thermal image
typedef struct {
    // The width of the thermal image in pixels
    uint16_t width;
    // The height of the thermal image in pixels
    uint16_t height;
    // The mode used to quantify the image data
    uint8_t mode;
    // The collection of thermal points
    flare16x_thermal_point* points;
} flare16x_thermal_image;

// Restore regular 32 or 64 bit struct boundaries
#pragma pack(pop)

// Represents a thermal image mask used to erase the crosshair
typedef struct {
    // The width of the thermal mask in pixels
    uint16_t width;
    // The height of the thermal mask in pixels
    uint16_t height;
    // The locator values for each mask pixel
    uint8_t* pixels;
} flare16x_thermal_mask;

// Represents a thermal context
typedef struct {
    // The cropped visible thermal image
    flare16x_canvas* visible_image;
    // The relative thermal image
    flare16x_thermal_image* thermal_image;
    // The text image
    flare16x_canvas* text_image;
    // The mask used to remove the crosshair
    flare16x_thermal_mask mask;
    // The average spot temperature in degrees celsius times 10 identified via OCR (-25.5°C -> -255)
    int16_t temperature_spot;
    // The minimum absolute temperature in the aperture spot in degrees celsius times 10 calculated
    int16_t temperature_spot_min;
    // The maximum absolute temperature in the aperture spot in degrees celsius times 10 calculated
    int16_t temperature_spot_max;
    // The error of the algorithm that determines the absolute temperatures from the center spot in degree celsius*100
    int16_t temperature_spot_error;
    // The minimum absolute temperature in the image in degrees celsius times 10 calculated
    int16_t temperature_min;
    // The maximum absolute temperature in the image in degrees celsius times 10 calculated
    int16_t temperature_max;
    // The emissivity times 100 identified via OCR (0.95 => 95)
    uint8_t emissivity;
    // The device model as defined in FLARE16X_LOCATOR_MODEL_*
    uint8_t device_model;
    // The y-coordinate of the aperture spot's upper left origin relative to the IR canvas
    uint16_t spot_x;
    // The y-coordinate of the aperture spot's upper left origin relative to the IR canvas
    uint16_t spot_y;
    // The width of the crosshair's aperture spot in pixels
    uint16_t spot_width;
    // The height of the crosshair's aperture spot in pixels
    uint16_t spot_height;
} flare16x_thermal;

// Enum describing the crosshair removal mode
enum {
    // The crosshair's pixels are replaced with the zero IR intensity value
    FLARE16X_THERMAL_INTERPOLATION_ZERO,
    // The crosshair's pixels are replaced with the lowest IR intensity value of the image
    FLARE16X_THERMAL_INTERPOLATION_MIN,
    // The crosshair's pixels are replaced with the median IR intensity value of the image
    FLARE16X_THERMAL_INTERPOLATION_MED,
    // The crosshair's pixels are replaced with the highest IR intensity value of the image
    FLARE16X_THERMAL_INTERPOLATION_MAX,
    // The crosshair's pixels are replaced with the median of the valid ones of the surrounding 2x2 square of pixels
    FLARE16X_THERMAL_INTERPOLATION_SQUARE_SMALL,
    // The crosshair's pixels are replaced with the median of the valid ones of the surrounding 6x6 square of pixels
    FLARE16X_THERMAL_INTERPOLATION_SQUARE_LARGE,
    // The crosshair's pixels are replaced with the weighted median of the valid ones of the surrounding 2x2 pixels
    FLARE16X_THERMAL_INTERPOLATION_SQUARE_WEIGHT,
    // The number of interpolation mode enum values
    FLARE16X_THERMAL_INTERPOLATION_COUNT
};

// Enum describing the border adding state machine
enum {
    FLARE16X_THERMAL_MASK_NONE,
    FLARE16X_THERMAL_MASK_BORDER,
    FLARE16X_THERMAL_MASK_FILL
};

// Modifies a raw thermal image point
#define flare16x_thermal_image_raw(x,y,image) (image)->points[(y) * (image)->width + (x)]

// Returns, if an offset from a point lies within the image data but lies outside of the crosshair
#define flare16x_thermal_valid(offset_x,offset_y,base_x,base_y,thermal) \
( ((int)(base_x)+(int)(offset_x)) >= 0 && ((int)(base_x)+(int)(offset_x)) < (thermal)->mask.width && \
((int)(base_y)+(int)(offset_y)) >= 0 && ((int)(base_y)+(int)(offset_y)) < (thermal)->mask.height && \
(thermal)->mask.pixels[((int)(base_y)+(int)(offset_y)) * (thermal)->mask.width + ((int)(base_x)+(int)(offset_x))] == \
FLARE16X_LOCATOR_DETECT_IMAGE )

// Initializes the thermal context using a locator struct
// Will destroy the locator struct supplied by moving its pointers to the thermal context
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_thermal_create(flare16x_locator* locator, flare16x_thermal* thermal);

// Runs OCR on the image and attempts to parse the OSD text
// If this function returns no error, it is safe to destroy the text image
flare16x_error flare16x_thermal_ocr(flare16x_thermal* thermal);

// Runs the palette analysis and converts the visible light image into relative IR data
// This step may take a while, as it calculates every pixel at least twice
// If this function returns no error, it is safe to destroy the thermal image
flare16x_error flare16x_thermal_process(flare16x_thermal* thermal, uint8_t interpolation_mode,
                                        uint8_t quantification_mode);

// Converts the relative thermal image into a visible image using the supplied palette
// Will overwrite any existing state and WILL leak memory if a previous state is re-used
flare16x_error flare16x_thermal_export(flare16x_thermal* thermal, uint8_t palette_index, flare16x_canvas* canvas);

// Adds a colored crosshair onto an exported thermal image using the mask
flare16x_error flare16x_thermal_crosshair(uint16_t crosshair_border, uint16_t crosshair_fill,
                                          flare16x_thermal* thermal, flare16x_canvas* canvas);

// Frees all resources used by a thermal struct including the canvas data
flare16x_error flare16x_thermal_destroy(flare16x_thermal* thermal);

// Allocates the resources for a new thermal image with exact quantification
flare16x_error flare16x_thermal_image_init(uint16_t width, uint16_t height, flare16x_thermal_image* image);

// Frees the resources used by a thermal image
flare16x_error flare16x_thermal_image_destroy(flare16x_thermal_image* image);

#endif //FLARE16X_THERMAL_H
